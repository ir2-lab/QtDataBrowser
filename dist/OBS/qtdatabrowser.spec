# set next variable to 1 if debug package is needed, else leave it to nil
# only for rpm builds
%define         debug_build                 %{nil}
%global         __strip                     /bin/true
%global         _lto_cflags                 %{nil}

%define         myname                      qtdatabrowser
%if ( 0%{?sle_version} == 150500 || 0%{?sle_version} == 150600 ) && 0%{?is_opensuse}
Name:           libQtDataBrowser0
%else
Name:           %{myname}
%endif
Version:        0
Release:        0
Summary:        Qt widget for exploring multi-dimensional scientific data
License:        GPL-3.0-or-later
Url:            https://github.com/ir2-lab/QtDataBrowser

Source0:        %{myname}.tar.gz

%if "%{_vendor}" == "debbuild"
Packager:       M. Axiotis <psaxioti@gmail.com>
   %if 0%{?ubuntu_version} >= 2204 || 0%{?debian_version} >= 1100
BuildRequires:  debhelper-compat = 13
   %else
BuildRequires:  debhelper-compat = 12
   %endif
BuildRequires:  debbuild-macros

BuildRequires:  qtbase5-dev
BuildRequires:  libqt5svg5-dev
%endif

BuildRequires:  cmake >= 3.16
BuildRequires:  %{!?_debbuild:gcc-c++}      %{?_debbuild:g++}
%if ( 0%{?sle_version} == 150500 || 0%{?sle_version} == 150600 ) && 0%{?is_opensuse}
BuildRequires:  libQMatPlotWidget0-devel
%else
BuildRequires:  qmatplotwidget-devel
%endif
%if 0%{?centos_version} && 0%{?centos_version} == 1000
BuildRequires:	 qwt-qt5-devel
%else
BuildRequires:  %{!?_debbuild:qwt6-qt5-devel}      %{?_debbuild:libqwt-qt5-dev}
%endif

%description
A Qt widget for exploring multi-dimensional scientific data.

%package        devel
Summary:        Development files for %{myname}

Requires:       %{name}%{?_isa} = %{version}-%{release}

%description    devel
Development files for Qt widget for exploring multi-dimensional scientific data.

###############################################################################################################################

%if "%{_vendor}" != "debbuild" && "%{debug_build}" == "1"
%debug_package
%else
%global         debug_package               %{nil}
%endif

###############################################################################################################################

%prep
%setup -q -n    %{myname}

%build
%cmake \
   -DCMAKE_INSTALL_PREFIX=%{_prefix} \
   -DCMAKE_BUILD_TYPE=RelWithDebInfo \
   -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
   %{nil}
%cmake_build

%install
%cmake_install

%if "%{_vendor}" == "debbuild" || "%{debug_build}" != "1"
strip --strip-unneeded %{buildroot}%{_libdir}/*.so
%endif

%post
/sbin/ldconfig

%postun
/sbin/ldconfig

%files
%license LICENSE
%doc README.md
%{_libdir}/lib*.so.*

%files devel
%{_libdir}/lib*.so
%{_includedir}/*
%dir %{_libdir}/cmake/QtDataBrowser
%{_libdir}/cmake/QtDataBrowser/*.cmake

%changelog
