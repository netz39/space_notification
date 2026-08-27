#include "service.h"

#include <signal.h>
#include <stddef.h>
#include <syslog.h>
#include <systemd/sd-daemon.h>

static volatile sig_atomic_t g_running = 1;

static void signal_handler(int signo)
{
    (void)signo;
    g_running = 0;
}

void service_start(const char *ident, const char *message)
{
    openlog(ident, LOG_CONS | LOG_PID, LOG_USER);
    syslog(LOG_INFO, "%s", message);
}

void service_stop(const char *message)
{
    syslog(LOG_INFO, "%s", message);
    closelog();
}

void service_setup_signals(void)
{
    struct sigaction sa = { .sa_handler = signal_handler };
    sigemptyset(&sa.sa_mask);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT,  &sa, NULL);
}

int service_is_running(void)
{
    return g_running;
}

void service_notify_ready(void)
{
    sd_notify(0, "READY=1");
}
