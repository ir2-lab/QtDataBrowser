%global         debug_package     %{nil}
%global         __strip           /bin/true
%global         _lto_cflags       %{nil}

Name:           qtdatabrowser
Version:        0
Release:        0
Summary:        Qt plot widget
License:        GPL-3.0-or-later
Url:            https://github.com/gapost/qmatplotwidget.git

Source0:        %{name}.tar.gz

%if "%{_vendor}" == "debbuild"
Packager:       M. Axiotis <psaxioti@gmail.com>
%endif

BuildRequires:  cmake >= 3.16
BuildRequires:  qmatplotwidget-devel

%if "%{_vendor}" == "debbuild"
   %if 0%{?ubuntu_version} >= 2204 || 0%{?debian_version} >= 1100
BuildRequires:  debhelper-compat = 13
   %else
BuildRequires:  debhelper-compat = 12
   %endif
BuildRequires:  debbuild-macros

BuildRequires:  g++
BuildRequires:  qtbase5-dev
BuildRequires:  libqwt-qt5-dev
BuildRequires:  libqt5svg5-dev

%else

BuildRequires:  gcc-c++
   %if 0%{?centos_version} && 0%{?centos_version} == 1000
BuildRequires:	 qwt-qt5-devel
   %else
BuildRequires:	 qwt6-qt5-devel
   %endif
%endif

%description
A Qt widget for exploring multi-dimensional scientific data.

%package        devel
Summary:        Development files for %{name}

%description    devel
Development files for Qt widget for exploring multi-dimensional scientific data.

%prep
%setup -q -n    %{name}

%build
%cmake \
   -DCMAKE_INSTALL_PREFIX=%{_prefix} \
   -DCMAKE_BUILD_TYPE=RelWithDebInfo \
   -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
   %{nil}
%cmake_build

%install
%cmake_install

%post
/sbin/ldconfig

%postun
/sbin/ldconfig

%files devel
%license LICENSE
%doc README.md
%{_libdir}/lib*.a
%{_includedir}/*
%dir %{_libdir}/cmake/QtDataBrowser
%{_libdir}/cmake/QtDataBrowser/*.cmake

%changelog
