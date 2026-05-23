#!/usr/bin/env bash
# Build the UCRT64 MSYS2 packages for QtDataBrowser.
# Must be run from the project root directory.
# Usage: bash dist/mingw64/build_pkg.sh <version>   (e.g. v0.2.0)

set -euo pipefail

# ---------------------------------------------------------------------------
# Arguments
# ---------------------------------------------------------------------------
VERSION="${1:-}"
if [[ -z "${VERSION}" ]]; then
  echo "Usage: $(basename "$0") <version>  (e.g. v0.2.0)" >&2
  exit 1
fi

# Strip leading 'v' to get the bare version number used by PKGBUILD
PKGVER="${VERSION#v}"

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"

# Must match the left-hand side of source=() in PKGBUILD
ARCHIVE_NAME="qtdatabrowser-${PKGVER}.tar.gz"
ARCHIVE_PATH="${BUILD_DIR}/${ARCHIVE_NAME}"

# Must match the directory referenced inside build() in PKGBUILD
GIT_PREFIX="QtDataBrowser-${PKGVER}/"

# ---------------------------------------------------------------------------
# Sanity checks
# ---------------------------------------------------------------------------
if ! command -v makepkg-mingw &>/dev/null; then
  echo "error: makepkg-mingw not found — run this script inside an MSYS2 shell" >&2
  exit 1
fi

# ---------------------------------------------------------------------------
# 1. Create source archive
# ---------------------------------------------------------------------------
mkdir -p "${BUILD_DIR}"

echo "==> Creating source archive from HEAD..."
git -C "${PROJECT_ROOT}" archive \
  --format=tar.gz \
  --prefix="${GIT_PREFIX}" \
  HEAD \
  -o "${ARCHIVE_PATH}"

# ---------------------------------------------------------------------------
# 2. Prepare PKGBUILD with correct version and checksum
# ---------------------------------------------------------------------------
SHA256=$(sha256sum "${ARCHIVE_PATH}" | awk '{print $1}')

echo "==> Preparing PKGBUILD (pkgver=${PKGVER}, sha256=${SHA256})..."
sed \
  -e "s/^pkgver=.*/pkgver=${PKGVER}/" \
  -e "s/sha256sums=('SKIP')/sha256sums=('${SHA256}')/" \
  "${SCRIPT_DIR}/PKGBUILD" > "${BUILD_DIR}/PKGBUILD"

# ---------------------------------------------------------------------------
# 3. Build UCRT64 package
# ---------------------------------------------------------------------------
echo "==> Building UCRT64 packages..."
cd "${BUILD_DIR}"
MINGW_ARCH=ucrt64 makepkg-mingw --noconfirm -sCLf

echo ""
echo "==> Done. Built packages:"
ls "${BUILD_DIR}"/*.pkg.tar.* 2>/dev/null || echo "  (none found)"
