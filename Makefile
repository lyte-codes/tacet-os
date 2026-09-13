# Tacet OS top-level targets. Component targets are delegated to each
# components/<name>/Makefile; image targets need podman (and root for disks).

COMPONENTS := $(patsubst %/Makefile,%,$(wildcard components/*/Makefile))
IMAGE      ?= localhost/tacet:dev
STAGE      := build/stage

.PHONY: all lint test packages build disk boot-test clean $(COMPONENTS)

all: lint test

lint:
	shellcheck -s bash ci/*.sh ci/lockfile/*.sh packaging/build-rpms.sh
	python3 -m py_compile ci/*.py ci/tests/*.py
	@if command -v hadolint >/dev/null; then hadolint Containerfile; else echo "hadolint not installed, skipped"; fi
	@for c in $(COMPONENTS); do $(MAKE) -C $$c lint || exit 1; done
	rm -rf $(STAGE); for c in $(COMPONENTS); do $(MAKE) -C $$c install DESTDIR=$(CURDIR)/$(STAGE) >/dev/null || exit 1; done
	cp -r overlays/usr/. $(STAGE)/usr/ && cp -r overlays/etc/. $(STAGE)/etc/
	cp /usr/lib/systemd/system/*.target /usr/lib/systemd/system/getty@.service /usr/lib/systemd/system/systemd-*.service $(STAGE)/usr/lib/systemd/system/
	systemd-analyze verify --root=$(CURDIR)/$(STAGE) $(STAGE)/usr/lib/systemd/system/tacet-*.service
	python3 ci/gen-packages-md.py --check

test:
	@for c in $(COMPONENTS); do $(MAKE) -C $$c test || exit 1; done
	python3 -m unittest discover -s ci/tests -q

packages:
	python3 ci/gen-packages-md.py > docs/PACKAGES.md

build:
	ci/build.sh $(IMAGE)

disk:
	sudo ci/make-disk.sh $(IMAGE) $(CURDIR)/output raw

boot-test:
	ci/boot-test.sh output/image/disk.raw

clean:
	rm -rf build output serial.log
