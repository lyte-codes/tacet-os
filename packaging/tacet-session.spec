# Built by packaging/build-rpms.sh inside the Containerfile's rpmbuild stage.
# `tacet_version` comes from components/tacet-session/VERSION.
Name:           tacet-session
Version:        %{tacet_version}
Release:        1%{?dist}
Summary:        Tacet TV session: gamescope and the shell on the seat
License:        TBD
URL:            https://github.com/lyte-codes/tacet-os
Source0:        %{name}-%{version}.tar.gz
BuildArch:      noarch

BuildRequires:  make
BuildRequires:  systemd-rpm-macros
Requires:       bash
Requires:       gamescope
Requires:       systemd
%{?systemd_requires}

%description
System unit and launcher that run gamescope on tty1 as the `tv` user with the
configured shell (Kodi by default) inside, plus the VM-only boot report used
by the CI boot test.

%prep
%autosetup -n %{name}-%{version}

%build

%install
%make_install PREFIX=%{_prefix}

%post
%systemd_post tacet-session.service tacet-boot-report.service

%preun
%systemd_preun tacet-session.service tacet-boot-report.service

%postun
%systemd_postun_with_restart tacet-session.service

%files
%doc README.md
%dir %{_libexecdir}/tacet
%{_libexecdir}/tacet/tacet-session
%{_libexecdir}/tacet/tacet-boot-report
%{_unitdir}/tacet-session.service
%{_unitdir}/tacet-boot-report.service

%changelog
* Sun Sep 13 2026 Tacet OS <noreply@tacet.invalid> - 0.1.0-1
- Initial package.
