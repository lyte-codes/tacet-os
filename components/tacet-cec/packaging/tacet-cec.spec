# Built by ../../packaging/build-rpms.sh inside the Containerfile's rpmbuild
# stage. `tacet_version` comes from components/tacet-cec/VERSION.
Name:           tacet-cec
Version:        %{tacet_version}
Release:        1%{?dist}
Summary:        HDMI-CEC to uinput bridge for Tacet OS
License:        TBD
URL:            https://github.com/lyte-codes/tacet-os
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  gcc
BuildRequires:  make
BuildRequires:  python3
BuildRequires:  libcec-devel
BuildRequires:  kernel-headers
BuildRequires:  systemd-rpm-macros
Requires:       libcec
Requires:       systemd-udev
%{?systemd_requires}

%description
Listens for HDMI-CEC key presses from the TV through libcec and replays them
as Linux input events on a virtual keyboard and gamepad, so the TV's own
remote drives Kodi, gamescope, Firefox and anything else on the box.

%prep
%autosetup -n %{name}-%{version}

%build
%make_build

%install
%make_install PREFIX=%{_prefix}

%check
make test

%post
%systemd_post tacet-cec.service
%udev_rules_update

%preun
%systemd_preun tacet-cec.service

%postun
%systemd_postun_with_restart tacet-cec.service
%udev_rules_update

%files
%doc README.md TESTING.md SPEC.md
%{_bindir}/tacet-cec
%{_unitdir}/tacet-cec.service
%{_udevrulesdir}/70-tacet-cec.rules
%{_modulesloaddir}/tacet-cec.conf
%dir %{_datadir}/tacet
%{_datadir}/tacet/cec.conf.default

%changelog
* Sun Sep 13 2026 Tacet OS <noreply@tacet.invalid> - 0.1.0-1
- Initial package.
