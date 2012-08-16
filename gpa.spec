%define name gpa
%define version 0.9.3

Summary: Graphical user interface for the GnuPG
Name: %{name}
Version: %{version}
Release: 1
License: GPLv3+
Group: Applications/System
Source0: ftp://ftp.gnupg.org/gcrypt/gpa/gpa-%{version}.tar.bz2
Source1: ftp://ftp.gnupg.org/gcrypt/gpa/gpa-%{version}.tar.bz2.sig
URL: http://www.gnupg.org/gpa.html
BuildRoot: %{_tmppath}/%{name}-root

%description
The GNU Privacy Assistant is a graphical user interface for the GNU Privacy
Guard (GnuPG). GnuPG is a system that provides you with privacy by
encrypting emails or other documents and with authentication of received
files by signature management.

%prep
%setup -q

%build
%configure
make

%install
rm -rf ${RPM_BUILD_ROOT}
install -Dpm 644 gpa.png \
	%{buildroot}%{_datadir}/pixmaps/gpa.png
install -Dpm 644 gpa.desktop \
	%{buildroot}%{_datadir}/applications/gpa.desktop
%makeinstall

%clean
rm -rf ${RPM_BUILD_ROOT}

%files
%defattr(-,root,root)
%doc ABOUT-NLS AUTHORS COPYING ChangeLog ChangeLog-2011 NEWS README THANKS TODO
%{_bindir}/gpa
%dir %{_datadir}/%{name}
%{_datadir}/%{name}/*.*
%{_datadir}/locale/*/*/gpa.mo
%{_datadir}/applications/gpa.desktop
%{_datadir}/pixmaps/gpa.png
%{_mandir}/man1/gpa.1.gz

%changelog
* Wed Aug 15 2012 Michael Petzold <michael.petzold@gmx.net> 0.9.3-1
- fixed source paths
- 'Copyright: GPL' -> 'License: GPLv3+'
- removed 'Requires: gnupg' (would have liked to specify 'gnupg OR gnupg2' but
  that doesn't seem possible)
- install: updated icon filename
- files: updated doc line, removed gtkrc, added gpa.1.gz

* Tue Mar 19 2002 Keith Hudson <kwhudson@netin.com>
- updated for 0.4.3
- added gnome menu entry

* Fri Aug  3 2001 Peter Hanecak <hanecak@megaloman.sk>
[0.4.1-1]
- initial spec
