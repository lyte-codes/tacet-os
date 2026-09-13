/* Virtual input devices through /dev/uinput (spec §4.1 step 4). */
#ifndef TACET_UINPUT_H
#define TACET_UINPUT_H

#include <stddef.h>
#include <stdint.h>

#define UINPUT_PATH "/dev/uinput"

/* Both return an fd, or -1 with err filled. */
int  uinput_open_keyboard(const char *name, const uint16_t *keys, size_t nkeys, char *err, size_t errlen);
int  uinput_open_gamepad(const char *name, char *err, size_t errlen);

int  uinput_key(int fd, uint16_t code, int value);     /* value 1 = down, 0 = up */
int  uinput_abs(int fd, uint16_t axis, int value);
int  uinput_syn(int fd);
void uinput_close(int fd);

#endif
