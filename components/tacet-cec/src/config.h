/* /etc/tacet/cec.conf parsing. Pure; unit-tested without hardware. */
#ifndef TACET_CONFIG_H
#define TACET_CONFIG_H

#include <stdbool.h>
#include <stddef.h>

#define CONFIG_DEVICE_NAME_MAX 13   /* CEC OSD name limit (LIBCEC_OSD_NAME_SIZE - 1) */
#define CONFIG_PATH_MAX        256

typedef enum { CONFIG_DEVICE_PLAYBACK = 4, CONFIG_DEVICE_RECORDING = 1 } config_device_type;

typedef struct config {
    char device_name[CONFIG_DEVICE_NAME_MAX + 1];
    config_device_type device_type;
    bool activate_source;   /* claim the TV input on start */
    bool wake_tv;           /* send power-on to the TV on start */
    int  hdmi_port;         /* 0 = autodetect, else 1..15 */
    char adapter[CONFIG_PATH_MAX];  /* "auto" or a device path */
    char keymap[CONFIG_PATH_MAX];   /* keymap file path */
} config;

void config_defaults(config *c);

/* Returns 1 if a value was set, 0 for blank/comment/unknown-key lines
 * (unknown keys are reported through err but not counted as failures),
 * -1 if the value was rejected (err filled, config unchanged). */
int  config_parse_line(config *c, const char *line, char *err, size_t errlen);

/* Number of rejected lines, or -1 if the file is unreadable. */
int  config_load_file(config *c, const char *path, char *err, size_t errlen);

#endif
