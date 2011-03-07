# Set svn_revision to SVN revision number if you want to do a SVN build.
# Parent source directory has to be "rockbox-rXXXXX"
%define svn_revision 0

%if 0%{?svn_revision}
# SVN rockbox build
Version:   r%{svn_revision}
%else
# Normal rockbox release
%define    major_version 3.8
Version:   3.8
%endif

Name:      rockbox
Summary:   High quality audio player
License:   GPL
Group:     Applications/Multimedia
Vendor:    rockbox.org
Release:   1%{?dist}
Url:       http://www.rockbox.org
Buildroot: %{_tmppath}/%{name}-%{version}-root
Requires:  SDL
BuildRequires: SDL-devel
# Note: rpm doesn't support 7z. You need to repack as .tar.bz2
# Source:    http://download.rockbox.org/release/%{major_version}/%{name}-%{version}.7z
Source:    %{name}-%{version}.tar.bz2
Prefix:    /opt/rockbox

%description
Rockbox open source high quality audio player

Features:
- Supports over 20 sound codecs:
  MP3, OGG, WAV, FLAC and many more
- Navigate music by folders or tag database
- Gapless playback and crossfading
- Ability to create your own themes
- Album art support

Need more reasons?
Find them here: http://www.rockbox.org/wiki/WhyRockbox

%prep
%setup -q

%build
mkdir build
cd build

../tools/configure --prefix=%{prefix} --target=sdlapp --lcdwidth=800 --lcdheight=480 --type=N

make %{?_smp_mflags}

%install
cd build
make PREFIX=$RPM_BUILD_ROOT/%{prefix} fullinstall

%clean
rm -fr $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%{prefix}/bin/rockbox
%{prefix}/lib/*
%{prefix}/share/*
