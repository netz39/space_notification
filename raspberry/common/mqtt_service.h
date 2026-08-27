#ifndef MQTT_SERVICE_H
#define MQTT_SERVICE_H

#include <mosquitto.h>

/**
 * Shared MQTT service infrastructure.
 *
 * Provides init/connect/loop/cleanup helpers used by both ledcontrol
 * and statusswitch so the boilerplate is not duplicated.
 */

/**
 * Initialise the mosquitto library and create a client instance.
 *
 * Logs errors to syslog. Returns NULL on failure; the library is still
 * initialised in that case and must be cleaned up with mqtt_service_cleanup.
 *
 * @param client_id  mosquitto client identifier string
 * @return           new mosquitto instance, or NULL on error
 */
struct mosquitto *mqtt_service_init(const char *client_id);

/**
 * Connect a mosquitto client to a broker.
 *
 * Logs errors to syslog.
 *
 * @param mosq      mosquitto instance created by mqtt_service_init
 * @param host      broker hostname
 * @param port      broker port
 * @param keepalive keepalive interval in seconds
 * @return          MOSQ_ERR_SUCCESS on success, mosquitto error code otherwise
 */
int mqtt_service_connect(struct mosquitto *mosq,
                         const char *host, int port, int keepalive);

/**
 * Run one iteration of the mosquitto network loop.
 *
 * If the loop reports an error, a reconnect is attempted.
 *
 * @param mosq     mosquitto instance; may be NULL (no-op)
 * @param timeout  loop timeout in milliseconds
 */
void mqtt_service_loop(struct mosquitto *mosq, int timeout);

/**
 * Disconnect, destroy the mosquitto client and clean up the library.
 *
 * Safe to call with mosq == NULL (only does library cleanup).
 *
 * @param mosq  mosquitto instance, or NULL
 */
void mqtt_service_cleanup(struct mosquitto *mosq);

#endif /* MQTT_SERVICE_H */
