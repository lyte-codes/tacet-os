# tacet-cec build. Plain make, C11, -Werror (spec §3).
#
#   make            build tacet-cec (needs libcec headers)
#   make test       build and run unit tests (no libcec, no hardware)
#   make lint       compile every translation unit that can be compiled here
#   make install    DESTDIR=... PREFIX=/usr

CC      ?= gcc
CFLAGS  ?= -O2 -g
CFLAGS  += -std=c11 -Wall -Wextra -Werror -D_GNU_SOURCE
LDFLAGS ?=

VERSION    := $(shell tr -d '[:space:]' < VERSION)
CFLAGS     += -DTACET_CEC_VERSION=\"$(VERSION)\"

INPUT_CODES_H ?= /usr/include/linux/input-event-codes.h
CEC_CFLAGS ?= $(shell pkg-config --cflags libcec 2>/dev/null)
CEC_LIBS   ?= $(shell pkg-config --libs libcec 2>/dev/null || echo -lcec)
HAVE_CEC   := $(shell echo '\#include <libcec/cecc.h>' | $(CC) $(CEC_CFLAGS) -E - >/dev/null 2>&1 && echo yes || echo no)

PREFIX       ?= /usr
DESTDIR      ?=
BINDIR        = $(PREFIX)/bin
UNITDIR       = $(PREFIX)/lib/systemd/system
UDEVRULESDIR  = $(PREFIX)/lib/udev/rules.d
MODLOADDIR    = $(PREFIX)/lib/modules-load.d
DATADIR       = $(PREFIX)/share/tacet

CORE_OBJS   = src/keymap.o src/config.o src/repeat.o src/log.o
DAEMON_OBJS = src/main.o src/cec.o src/uinput.o $(CORE_OBJS)
TEST_OBJS   = tests/main.o tests/test_keymap.o tests/test_config.o tests/test_repeat.o

.PHONY: all test lint install clean

all: tacet-cec

src/keycodes.h: tools/gen-keycodes.py $(INPUT_CODES_H)
	python3 tools/gen-keycodes.py $(INPUT_CODES_H) > $@.tmp && mv $@.tmp $@

src/keymap.o: src/keycodes.h
src/cec.o: CFLAGS += $(CEC_CFLAGS)

%.o: %.c src/keymap.h src/config.h src/repeat.h src/log.h src/cec.h src/uinput.h
	$(CC) $(CFLAGS) -c $< -o $@

tacet-cec: $(DAEMON_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(DAEMON_OBJS) $(CEC_LIBS)

tests/run-tests: $(TEST_OBJS) $(CORE_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(TEST_OBJS) $(CORE_OBJS)

test: tests/run-tests
	./tests/run-tests

lint: src/keycodes.h
	@for f in src/main.c src/uinput.c src/keymap.c src/config.c src/repeat.c src/log.c tests/*.c; do \
	  $(CC) $(CFLAGS) -fsyntax-only $$f || exit 1; done
ifeq ($(HAVE_CEC),yes)
	$(CC) $(CFLAGS) $(CEC_CFLAGS) -fsyntax-only src/cec.c
else
	@echo "lint: libcec headers not found, skipped src/cec.c"
endif
	python3 -m py_compile tools/gen-keycodes.py

install: tacet-cec
	install -d $(DESTDIR)$(BINDIR) $(DESTDIR)$(UNITDIR) $(DESTDIR)$(UDEVRULESDIR) \
	           $(DESTDIR)$(MODLOADDIR) $(DESTDIR)$(DATADIR)
	install -m 0755 tacet-cec $(DESTDIR)$(BINDIR)/tacet-cec
	install -m 0644 packaging/tacet-cec.service $(DESTDIR)$(UNITDIR)/tacet-cec.service
	install -m 0644 packaging/70-tacet-cec.rules $(DESTDIR)$(UDEVRULESDIR)/70-tacet-cec.rules
	install -m 0644 packaging/tacet-cec-modules.conf $(DESTDIR)$(MODLOADDIR)/tacet-cec.conf
	install -m 0644 packaging/cec.conf.default $(DESTDIR)$(DATADIR)/cec.conf.default

clean:
	rm -f tacet-cec tests/run-tests src/*.o tests/*.o src/keycodes.h src/keycodes.h.tmp
