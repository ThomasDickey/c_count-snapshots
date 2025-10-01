Summary: c_count - C-language line counter
# $Id: c_count.spec,v 1.17 2025/10/01 23:51:30 tom Exp $
Name: c_count
Version: 7.25
Release: 1
License: MIT
Group: Applications/Development
URL: ftp://invisible-island.net/%{name}
Source0: %{name}-%{version}.tgz
Vendor: Thomas E. Dickey
Packager: Thomas E. Dickey <dickey@invisible-island.net>

%description
C_count  counts  lines  and  statements in C-language source files.  It
provides related statistics on the amount of whitespace,  comments  and
code.   C_count  also  shows  the  presence  of  unbalanced (or nested)
comments, unbalanced quotation marks and illegal characters.

%prep

%define debug_package %{nil}

%setup -q -n %{name}-%{version}

%build

INSTALL_PROGRAM='${INSTALL}' \
	./configure \
		--prefix=%{_prefix} \
		--bindir=%{_bindir} \
		--mandir=%{_mandir}

make

%install
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

make install                    DESTDIR=$RPM_BUILD_ROOT

strip $RPM_BUILD_ROOT%{_bindir}/%{name}

%clean
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%{_bindir}/%{name}
%{_mandir}/man1/%{name}.*

%changelog
# each patch should add its ChangeLog entries here

* Wed Oct 01 2025 Thomas E. Dickey
- testing c_count 7.25-1

* Fri May 11 2018 Thomas Dickey
- suppress debug-symbols

* Fri Jul 16 2010 Thomas Dickey
- initial version
