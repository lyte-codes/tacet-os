/* Virtual keyboard through /dev/uinput. */
#ifndef TACET_UINPUT_H
#define TACET_UINPUT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Creates the device with every key the keymap module knows enabled.
 * Returns the fd, or -1 with err filled. */
int  uinput_open(const char *path, const char *name, char *err, size_t errlen);
int  uinput_emit(int fd, uint16_t key, bool pressed);
void uinput_close(int fd);

#endif
