%define name gpa
%define version 0.4.3

Summary: Graphical user interface for the GnuPG.
Name: %{name}
Version: %{version}
Release: 1
Copyright: GPL
Group: Applications/Cryptography
Source0: ftp://ftp.gnupg.org/gcrypt/alpha/gpa/gpa-%{version}.tar.gz
Source1: ftp://ftp.gnupg.org/gcrypt/alpha/gpa/gpa-%{version}.tar.gz.sig
URL: http://www.gnupg.org/gpa.html
BuildRoot: %{_tmppath}/%{name}-root
Requires: gnupg

%description
The GNU Privacy Assistant is a graphical user interface for the GNU Privacy
Guard (GnuPG). GnuPG is a system that provides you with privacy by
encrypting emails or other documents and with authentication of received
files by signature management.

%prep
%setup

%build
%configure
make

%install
rm -rf ${RPM_BUILD_ROOT}
mkdir -p ${RPM_BUILD_ROOT}
install -D -m 644 gpa-logo-48x48.png \
%{buildroot}%{_datadir}/pixmaps/gpa-logo-48x48.png
install -D -m 644 gpa.desktop \
%{buildroot}%{_datadir}/gnome/apps/Applications/gpa.desktop
%makeinstall

%clean
rm -rf ${RPM_BUILD_ROOT}

%files
%defattr(-,root,root)
%doc ABOUT-NLS AUTHORS ChangeLog INSTALL NEWS README README-alpha THANKS TODO
%{_bindir}/gpa
%dir %{_datadir}/%{name}
%{_datadir}/%{name}/*.*
%config %{_datadir}/%{name}/gtkrc
%{_datadir}/locale/*/*/gpa.mo
%{_datadir}/gnome/apps/Applications/gpa.desktop
%{_datadir}/pixmaps/gpa.png

%changelog
* Tue Mar 19 2002 Keith Hudson <kwhudson@netin.com
- updated for 0.4.3
- added gnome menu entry
* Fri Aug  3 2001 Peter Hanecak <hanecak@megaloman.sk>
[0.4.1-1]
- initial spec
 
