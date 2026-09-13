/* CEC user-control code -> input event mapping (spec §5). Pure data. */
#ifndef TACET_KEYMAP_H
#define TACET_KEYMAP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define KEYMAP_CEC_CODES 256
#define KEYMAP_UNMAPPED  0

typedef struct keymap_entry {
    uint16_t key;        /* Linux KEY_ or BTN_ code on the keyboard device, 0 = unmapped */
    bool     no_repeat;
} keymap_entry;

typedef struct keymap {
    keymap_entry entry[KEYMAP_CEC_CODES];
} keymap;

/* Gamepad mirror (spec §5.2): what a CEC code does on the gamepad device. */
typedef enum { GP_NONE = 0, GP_HAT_X, GP_HAT_Y, GP_BUTTON } gamepad_kind;
typedef struct gamepad_action {
    gamepad_kind kind;
    int16_t      value;   /* hat: -1 or +1; button: BTN_* code */
} gamepad_action;

void                keymap_defaults(keymap *m);
const keymap_entry *keymap_lookup(const keymap *m, int cec_code);
gamepad_action      keymap_gamepad(int cec_code);

/* Applies one "[keymap]" line: `0x41 = KEY_VOLUMEUP[,no_repeat]` or `= none`.
 * CEC codes are numbers (0x41, 65) or, as an extension, libcec names
 * (VOLUME_UP). Returns 1 on change, 0 for blank/comment, -1 on error. */
int  keymap_apply(keymap *m, const char *lhs, const char *rhs, char *err, size_t errlen);

int         keymap_cec_code(const char *name);       /* -1 unknown */
const char *keymap_cec_name(int cec_code);           /* NULL unknown */
int         keycode_from_name(const char *name);     /* KEY_ or BTN_ name, or a number; -1 unknown */
const char *keycode_to_name(int code);               /* NULL unknown */

/* Unique keyboard codes referenced by the map (for uinput setup).
 * Returns the count written to out (at most max). */
size_t keymap_referenced_keys(const keymap *m, uint16_t *out, size_t max);

void keymap_dump(const keymap *m, bool gamepad, FILE *f);

#endif
