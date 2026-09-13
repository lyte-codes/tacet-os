#include "uinput.h"
#include "keymap.h"

#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

int uinput_open(const char *path, const char *name, char *err, size_t errlen)
{
    int fd = open(path, O_WRONLY | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0) {
        snprintf(err, errlen, "open %s: %s", path, strerror(errno));
        return -1;
    }
    if (ioctl(fd, UI_SET_EVBIT, EV_KEY) < 0 || ioctl(fd, UI_SET_EVBIT, EV_SYN) < 0 ||
        ioctl(fd, UI_SET_EVBIT, EV_REP) < 0) {
        snprintf(err, errlen, "UI_SET_EVBIT: %s", strerror(errno));
        goto fail;
    }
    for (size_t i = 0; i < keymap_known_key_count(); i++) {
        if (ioctl(fd, UI_SET_KEYBIT, keymap_known_key(i)) < 0) {
            snprintf(err, errlen, "UI_SET_KEYBIT %u: %s", keymap_known_key(i), strerror(errno));
            goto fail;
        }
    }

    struct uinput_setup us;
    memset(&us, 0, sizeof(us));
    us.id.bustype = BUS_VIRTUAL;
    us.id.vendor  = 0x7461;   /* "ta" */
    us.id.product = 0x6365;   /* "ce" */
    us.id.version = 1;
    snprintf(us.name, sizeof(us.name), "%s", name);
    if (ioctl(fd, UI_DEV_SETUP, &us) < 0) {
        snprintf(err, errlen, "UI_DEV_SETUP: %s", strerror(errno));
        goto fail;
    }
    if (ioctl(fd, UI_DEV_CREATE) < 0) {
        snprintf(err, errlen, "UI_DEV_CREATE: %s", strerror(errno));
        goto fail;
    }
    return fd;

fail:
    close(fd);
    return -1;
}

static int emit(int fd, uint16_t type, uint16_t code, int32_t value)
{
    struct input_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.type = type;
    ev.code = code;
    ev.value = value;
    return write(fd, &ev, sizeof(ev)) == (ssize_t)sizeof(ev) ? 0 : -1;
}

int uinput_emit(int fd, uint16_t key, bool pressed)
{
    if (emit(fd, EV_KEY, key, pressed ? 1 : 0) < 0)
        return -1;
    return emit(fd, EV_SYN, SYN_REPORT, 0);
}

void uinput_close(int fd)
{
    if (fd < 0)
        return;
    ioctl(fd, UI_DEV_DESTROY);
    close(fd);
}
