%define name gpa
%define version 0.4.1

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

%changelog
* Fri Aug  3 2001 Peter Hanecak <hanecak@megaloman.sk>
[0.4.1-1]
- initial spec
