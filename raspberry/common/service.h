#ifndef SERVICE_H
#define SERVICE_H

/**
 * Shared service lifecycle helpers.
 *
 * Provides syslog lifecycle wrappers, POSIX signal handling, and
 * systemd readiness notification.
 */

/**
 * Open the system log and log a startup message.
 *
 * @param ident    syslog identifier (process name)
 * @param message  human-readable startup message written at LOG_INFO
 */
void service_start(const char *ident, const char *message);

/**
 * Log a shutdown message and close the system log.
 *
 * @param message  human-readable shutdown message written at LOG_INFO
 */
void service_stop(const char *message);

/**
 * Install signal handlers for SIGTERM and SIGINT.
 *
 * Handlers only set an internal flag; all cleanup happens in normal
 * program flow after the run loop exits.
 */
void service_setup_signals(void);

/**
 * Return non-zero while the service should keep running.
 *
 * Becomes zero when a SIGTERM or SIGINT has been received.
 */
int service_is_running(void);

/**
 * Notify systemd that the service is ready (READY=1).
 *
 * Uses sd_notify(). Safe to call when not running under systemd.
 */
void service_notify_ready(void);

#endif /* SERVICE_H */
