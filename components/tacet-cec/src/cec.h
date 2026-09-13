/* libcec wrapper (spec §11: open, callbacks, active source, reconnect).
 * Everything libcec-specific lives in cec.c; main.c never sees cecc.h. */
#ifndef TACET_CEC_H
#define TACET_CEC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

typedef enum {
    CEC_EV_KEY_PRESS,      /* code = CEC user control code */
    CEC_EV_KEY_RELEASE,
    CEC_EV_DISCONNECT,     /* adapter gone; caller must cec_close() and retry */
    CEC_EV_TV_WAKE,        /* TV reported power on or asked who the active source is */
    CEC_EV_TV_STANDBY,
    CEC_EV_SOURCE_LOST,    /* another device became active source */
} cec_event_kind;

typedef struct cec_event {
    cec_event_kind kind;
    uint8_t        code;
} cec_event;

/* Called from libcec's threads. Must be cheap and thread-safe. */
typedef void (*cec_event_fn)(void *user, const cec_event *ev);

typedef struct cec_settings {
    const char *adapter;          /* "auto" or a path */
    int         device_type;      /* 4 = playback, 1 = recording */
    uint16_t    physical_address; /* 0xFFFF = autodetect */
    const char *device_name;
    bool        activate_source;  /* claim the input when the TV asks / on open */
} cec_settings;

typedef struct cec_conn cec_conn;

cec_conn   *cec_open(const cec_settings *s, cec_event_fn fn, void *user, char *err, size_t errlen);
void        cec_close(cec_conn *c);
const char *cec_port(const cec_conn *c);
int         cec_conn_logical_address(const cec_conn *c);
int         cec_set_active_source(cec_conn *c);

/* Prints one line per adapter to out. Returns the count, or -1 if libcec
 * could not be initialised at all. Zero adapters is not an error. */
int         cec_list_adapters(FILE *out);

#endif
