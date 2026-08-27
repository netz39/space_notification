#ifndef SERVICE_H
#define SERVICE_H

#include <syslog.h>

/**
 * Shared service lifecycle helpers.
 *
 * Thin wrappers around syslog that log a start and stop message and
 * open/close the system log with consistent options.
 */

/**
 * Open the system log and log a startup message.
 *
 * @param ident    syslog identifier (process name)
 * @param message  human-readable startup message written at LOG_INFO
 */
static void service_start(const char *ident, const char *message)
{
    openlog(ident, LOG_CONS | LOG_PID, LOG_USER);
    syslog(LOG_INFO, "%s", message);
}

/**
 * Log a shutdown message and close the system log.
 *
 * @param message  human-readable shutdown message written at LOG_INFO
 */
static void service_stop(const char *message)
{
    syslog(LOG_INFO, "%s", message);
    closelog();
}

#endif /* SERVICE_H */
