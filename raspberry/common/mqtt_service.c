#include "mqtt_service.h"

#include <syslog.h>
#include <errno.h>

struct mosquitto *mqtt_service_init(const char *client_id)
{
    mosquitto_lib_init();

    struct mosquitto *mosq = mosquitto_new(client_id, true, NULL);
    if (!mosq) {
        syslog(LOG_ERR, "MQTT error %d (%s)!", errno, mosquitto_strerror(errno));
        return NULL;
    }

    return mosq;
}

int mqtt_service_connect(struct mosquitto *mosq,
                         const char *host, int port, int keepalive)
{
    const int ret = mosquitto_connect(mosq, host, port, keepalive);
    if (ret == MOSQ_ERR_SUCCESS)
        syslog(LOG_INFO, "MQTT connection to %s established.", host);
    else
        syslog(LOG_ERR, "MQTT error %d (%s)!", ret, mosquitto_strerror(ret));

    return ret;
}

void mqtt_service_loop(struct mosquitto *mosq, int timeout)
{
    if (!mosq)
        return;

    const int ret = mosquitto_loop(mosq, timeout, 1);
    if (ret) {
        syslog(LOG_ERR, "MQTT loop error, reconnecting.");
        mosquitto_reconnect(mosq);
    }
}

void mqtt_service_cleanup(struct mosquitto *mosq)
{
    if (mosq) {
        mosquitto_disconnect(mosq);
        mosquitto_destroy(mosq);
    }
    mosquitto_lib_cleanup();
}
