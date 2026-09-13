PREFIX     ?= /usr
DESTDIR    ?=
LIBEXECDIR  = $(PREFIX)/libexec/tacet
UNITDIR     = $(PREFIX)/lib/systemd/system

SCRIPTS = tacet-session tacet-boot-report
UNITS   = systemd/tacet-session.service systemd/tacet-boot-report.service

.PHONY: all install test lint

all:

install:
	install -d $(DESTDIR)$(LIBEXECDIR) $(DESTDIR)$(UNITDIR)
	install -m 0755 $(SCRIPTS) $(DESTDIR)$(LIBEXECDIR)/
	install -m 0644 $(UNITS) $(DESTDIR)$(UNITDIR)/

test:
	bash tests/run.sh

lint:
	shellcheck -s bash $(SCRIPTS) tests/run.sh
