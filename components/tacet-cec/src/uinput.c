#include "uinput.h"

#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

static int open_node(char *err, size_t errlen)
{
    int fd = open(UINPUT_PATH, O_WRONLY | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0)
        snprintf(err, errlen, "open %s: %s", UINPUT_PATH, strerror(errno));
    return fd;
}

static int finish(int fd, const char *name, uint16_t product, char *err, size_t errlen)
{
    struct uinput_setup us;
    memset(&us, 0, sizeof(us));
    us.id.bustype = BUS_VIRTUAL;
    us.id.vendor = 0x7463;      /* "tc" */
    us.id.product = product;
    us.id.version = 1;
    snprintf(us.name, sizeof(us.name), "%s", name);
    if (ioctl(fd, UI_DEV_SETUP, &us) < 0) {
        snprintf(err, errlen, "UI_DEV_SETUP: %s", strerror(errno));
        return -1;
    }
    if (ioctl(fd, UI_DEV_CREATE) < 0) {
        snprintf(err, errlen, "UI_DEV_CREATE: %s", strerror(errno));
        return -1;
    }
    return 0;
}

int uinput_open_keyboard(const char *name, const uint16_t *keys, size_t nkeys, char *err, size_t errlen)
{
    int fd = open_node(err, errlen);
    if (fd < 0)
        return -1;
    if (ioctl(fd, UI_SET_EVBIT, EV_KEY) < 0 || ioctl(fd, UI_SET_EVBIT, EV_SYN) < 0) {
        snprintf(err, errlen, "UI_SET_EVBIT: %s", strerror(errno));
        goto fail;
    }
    for (size_t i = 0; i < nkeys; i++) {
        if (ioctl(fd, UI_SET_KEYBIT, keys[i]) < 0) {
            snprintf(err, errlen, "UI_SET_KEYBIT %u: %s", keys[i], strerror(errno));
            goto fail;
        }
    }
    if (finish(fd, name, 0x0001, err, errlen) < 0)
        goto fail;
    return fd;
fail:
    close(fd);
    return -1;
}

int uinput_open_gamepad(const char *name, char *err, size_t errlen)
{
    static const uint16_t buttons[] = {
        BTN_SOUTH, BTN_EAST, BTN_NORTH, BTN_WEST, BTN_TL, BTN_TR,
        BTN_SELECT, BTN_START, BTN_MODE,
    };
    int fd = open_node(err, errlen);
    if (fd < 0)
        return -1;
    if (ioctl(fd, UI_SET_EVBIT, EV_KEY) < 0 || ioctl(fd, UI_SET_EVBIT, EV_ABS) < 0 ||
        ioctl(fd, UI_SET_EVBIT, EV_SYN) < 0) {
        snprintf(err, errlen, "UI_SET_EVBIT: %s", strerror(errno));
        goto fail;
    }
    for (size_t i = 0; i < sizeof(buttons) / sizeof(buttons[0]); i++) {
        if (ioctl(fd, UI_SET_KEYBIT, buttons[i]) < 0) {
            snprintf(err, errlen, "UI_SET_KEYBIT %u: %s", buttons[i], strerror(errno));
            goto fail;
        }
    }
    for (uint16_t axis = ABS_HAT0X; axis <= ABS_HAT0Y; axis++) {
        struct uinput_abs_setup as;
        memset(&as, 0, sizeof(as));
        as.code = axis;
        as.absinfo.minimum = -1;
        as.absinfo.maximum = 1;
        if (ioctl(fd, UI_SET_ABSBIT, axis) < 0 || ioctl(fd, UI_ABS_SETUP, &as) < 0) {
            snprintf(err, errlen, "UI_ABS_SETUP %u: %s", axis, strerror(errno));
            goto fail;
        }
    }
    if (finish(fd, name, 0x0002, err, errlen) < 0)
        goto fail;
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

int uinput_key(int fd, uint16_t code, int value) { return emit(fd, EV_KEY, code, value); }
int uinput_abs(int fd, uint16_t axis, int value) { return emit(fd, EV_ABS, axis, value); }
int uinput_syn(int fd) { return emit(fd, EV_SYN, SYN_REPORT, 0); }

void uinput_close(int fd)
{
    if (fd < 0)
        return;
    ioctl(fd, UI_DEV_DESTROY);
    close(fd);
}
