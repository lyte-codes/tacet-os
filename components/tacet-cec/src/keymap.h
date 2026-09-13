/* CEC user-control code -> Linux input key code table.
 *
 * Pure data and parsing; no libcec, no uinput, so it is unit-testable on any
 * machine. See SPEC.md §3 for the mapping rules.
 */
#ifndef TACET_KEYMAP_H
#define TACET_KEYMAP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define KEYMAP_CEC_CODES 256
#define KEYMAP_UNMAPPED  0      /* KEY_RESERVED: the CEC code is ignored */

/* X11 keycodes are 8..255, so an evdev code above this cannot reach an X11
 * client (Kodi runs under gamescope's XWayland). */
#define KEYMAP_X11_MAX_KEY 247

typedef struct keymap {
    uint16_t key[KEYMAP_CEC_CODES];
} keymap;

void        keymap_defaults(keymap *m);
uint16_t    keymap_lookup(const keymap *m, int cec_code);

/* Names. CEC names accept "SELECT" or "CEC_USER_CONTROL_CODE_SELECT",
 * case-insensitively, or a number (0x00, 0). Key names accept "KEY_ENTER" or
 * "ENTER", case-insensitively, or a number. -1 means unknown. */
int         keymap_cec_code(const char *name);
const char *keymap_cec_name(int cec_code);
int         keymap_key_code(const char *name);
const char *keymap_key_name(int key_code);
bool        keymap_key_fits_x11(int key_code);

/* Iterate the known key table (for enabling keys on the uinput device). */
size_t      keymap_known_key_count(void);
uint16_t    keymap_known_key(size_t index);

/* One "CEC_NAME KEY_NAME" line. "none" as the key unmaps the code.
 * Returns 1 if a mapping changed, 0 for blank/comment lines, -1 on error
 * (err filled, map unchanged). */
int         keymap_parse_line(keymap *m, const char *line, char *err, size_t errlen);

/* Applies every line of a file on top of the current map.
 * Returns the number of rejected lines (0 = clean), or -1 if the file could
 * not be read. err holds the last error, prefixed with its line number. */
int         keymap_load_file(keymap *m, const char *path, char *err, size_t errlen);

#endif
