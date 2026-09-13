#include "cec.h"
#include "log.h"

#include <libcec/cecc.h>
#include <stdlib.h>
#include <string.h>

struct cec_conn {
    libcec_connection_t  conn;
    libcec_configuration cfg;
    ICECCallbacks        cb;
    cec_event_fn         fn;
    void                *user;
    char                 port[1024];
};

static void emit(cec_conn *c, cec_event_kind kind, uint8_t code)
{
    cec_event ev = { kind, code };
    c->fn(c->user, &ev);
}

static void on_key(void *param, const cec_keypress *key)
{
    cec_conn *c = param;
    emit(c, key->duration == 0 ? CEC_EV_KEY_PRESS : CEC_EV_KEY_RELEASE, (uint8_t)key->keycode);
}

static void on_log(void *param, const cec_log_message *msg)
{
    (void)param;
    switch (msg->level) {
    case CEC_LOG_ERROR:   log_error("libcec: %s", msg->message); break;
    case CEC_LOG_WARNING: log_warn("libcec: %s", msg->message); break;
    case CEC_LOG_NOTICE:  log_info("libcec: %s", msg->message); break;
    default:              log_debug("libcec: %s", msg->message); break;   /* TRAFFIC, DEBUG */
    }
}

static void on_alert(void *param, const libcec_alert alert, const libcec_parameter p)
{
    cec_conn *c = param;
    const char *detail = p.paramType == CEC_PARAMETER_TYPE_STRING && p.paramData ? p.paramData : "";
    switch (alert) {
    case CEC_ALERT_CONNECTION_LOST:
        log_warn("adapter connection lost %s", detail);
        emit(c, CEC_EV_DISCONNECT, 0);
        break;
    case CEC_ALERT_PERMISSION_ERROR:
    case CEC_ALERT_PORT_BUSY:
        log_error("adapter unusable (%s) %s", alert == CEC_ALERT_PORT_BUSY ? "port busy" : "permission denied", detail);
        emit(c, CEC_EV_DISCONNECT, 0);
        break;
    case CEC_ALERT_PHYSICAL_ADDRESS_ERROR:
        log_warn("physical address error %s", detail);
        break;
    case CEC_ALERT_TV_POLL_FAILED:
        log_debug("TV poll failed %s", detail);
        break;
    default:
        log_debug("libcec alert %d %s", (int)alert, detail);
    }
}

static void on_command(void *param, const cec_command *cmd)
{
    cec_conn *c = param;
    if (!cmd->opcode_set)
        return;
    log_debug("cec frame %X->%X opcode 0x%02X len %u", (unsigned)cmd->initiator,
              (unsigned)cmd->destination, (unsigned)cmd->opcode, (unsigned)cmd->parameters.size);
    if (cmd->initiator != CECDEVICE_TV)
        return;
    switch (cmd->opcode) {
    case CEC_OPCODE_STANDBY:
        emit(c, CEC_EV_TV_STANDBY, 0);
        break;
    case CEC_OPCODE_REQUEST_ACTIVE_SOURCE:
        emit(c, CEC_EV_TV_WAKE, 0);
        break;
    case CEC_OPCODE_REPORT_POWER_STATUS:
        if (cmd->parameters.size >= 1 && cmd->parameters.data[0] == CEC_POWER_STATUS_ON)
            emit(c, CEC_EV_TV_WAKE, 0);
        break;
    default:
        break;
    }
}

static void on_source_activated(void *param, const cec_logical_address addr, const uint8_t activated)
{
    cec_conn *c = param;
    log_info("active source %s (logical address %X)", activated ? "gained" : "lost", (unsigned)addr);
    if (!activated)
        emit(c, CEC_EV_SOURCE_LOST, 0);
}

static libcec_connection_t init(libcec_configuration *cfg, const cec_settings *s, cec_conn *c)
{
    libcec_clear_configuration(cfg);
    cfg->clientVersion = LIBCEC_VERSION_CURRENT;
    snprintf(cfg->strDeviceName, LIBCEC_OSD_NAME_SIZE, "%s", s && s->device_name ? s->device_name : "Tacet");
    cfg->deviceTypes.types[0] = s ? (cec_device_type)s->device_type : CEC_DEVICE_TYPE_PLAYBACK_DEVICE;
    cfg->bActivateSource = s ? (s->activate_source ? 1 : 0) : 0;
    if (s && s->physical_address != CEC_INVALID_PHYSICAL_ADDRESS)
        cfg->iPhysicalAddress = s->physical_address;
    if (c) {
        memset(&c->cb, 0, sizeof(c->cb));
        c->cb.keyPress = on_key;
        c->cb.logMessage = on_log;
        c->cb.alert = on_alert;
        c->cb.commandReceived = on_command;
        c->cb.sourceActivated = on_source_activated;
        cfg->callbackParam = c;
        cfg->callbacks = &c->cb;
    }
    return libcec_initialise(cfg);
}

cec_conn *cec_open(const cec_settings *s, cec_event_fn fn, void *user, char *err, size_t errlen)
{
    cec_conn *c = calloc(1, sizeof(*c));
    if (!c) {
        snprintf(err, errlen, "out of memory");
        return NULL;
    }
    c->fn = fn;
    c->user = user;
    c->conn = init(&c->cfg, s, c);
    if (!c->conn) {
        snprintf(err, errlen, "libcec_initialise failed");
        free(c);
        return NULL;
    }

    if (strcmp(s->adapter, "auto") == 0) {
        cec_adapter adapters[8];
        int8_t n = libcec_find_adapters(c->conn, adapters, 8, NULL);
        if (n <= 0) {
            snprintf(err, errlen, "no CEC adapters found");
            libcec_destroy(c->conn);
            free(c);
            return NULL;
        }
        snprintf(c->port, sizeof(c->port), "%s", adapters[0].comm);
    } else {
        snprintf(c->port, sizeof(c->port), "%s", s->adapter);
    }

    if (!libcec_open(c->conn, c->port, 5000)) {
        snprintf(err, errlen, "could not open adapter %s", c->port);
        libcec_destroy(c->conn);
        free(c);
        return NULL;
    }
    return c;
}

void cec_close(cec_conn *c)
{
    if (!c)
        return;
    libcec_close(c->conn);
    libcec_destroy(c->conn);
    free(c);
}

const char *cec_port(const cec_conn *c) { return c->port; }

int cec_conn_logical_address(const cec_conn *c)
{
    cec_logical_addresses la = libcec_get_logical_addresses(c->conn);
    return la.primary;
}

int cec_set_active_source(cec_conn *c)
{
    return libcec_set_active_source(c->conn, c->cfg.deviceTypes.types[0]) ? 0 : -1;
}

int cec_list_adapters(FILE *out)
{
    libcec_configuration cfg;
    libcec_connection_t conn = init(&cfg, NULL, NULL);
    if (!conn) {
        fprintf(out, "libcec could not be initialised\n");
        return -1;
    }
    cec_adapter adapters[8];
    int8_t n = libcec_find_adapters(conn, adapters, 8, NULL);
    if (n <= 0)
        fprintf(out, "no CEC adapters found\n");
    for (int8_t i = 0; i < n; i++)
        fprintf(out, "%s\t%s\n", adapters[i].comm, adapters[i].path);
    libcec_destroy(conn);
    return n < 0 ? 0 : n;
}
