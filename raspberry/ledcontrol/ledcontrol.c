#include <stdint.h>
#include <stdbool.h>

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include "../common/i2c_command.h"
#include "../common/mqtt_service.h"
#include "../common/service.h"

#define I2C_ADDR_AMPEL		0x20

const char* MQTT_HOST 		= "platon.n39.eu";
const int   MQTT_PORT 		= 1883;
const char* MQTT_AMPEL_TOPIC	= "Netz39/Things/Ampel/Light";
const char* MQTT_NIGHTMODE_TOPIC= "Netz39/Nightmode";

/* Duration (seconds) to keep LEDs on after an ampel change during nightmode */
#define NIGHTMODE_VISIBILITY_SECS 2

struct ampel_state_t {
  bool red;
  bool green;
  bool blink;
};

/* Global nightmode state */
static bool nightmode_active = false;

/* Most recently requested ampel state (independent of physical output) */
static struct ampel_state_t requested_state = { false, false, false };

/* Monotonic time (seconds) when the brief nightmode visibility expires;
 * 0 means no timer is running. */
static time_t nightmode_show_until = 0;

///// I2C stuff /////

/**
  * This record contains the I2C file descriptors we use in this program
  */
struct I2C_descriptors {
  int ampel;
} I2C_fd;

#define I2C_FD_AMPEL (I2C_fd.ampel)

/**
  * Initialize all I2C channels and store the file descriptors.
  */
void I2C_init(void) {
  I2C_fd.ampel = I2C_setup_fd(I2C_ADDR_AMPEL);
}

#define AMPEL_CMD_RESET		0x00
#define AMPEL_CMD_GETLIGHT	0x01
#define AMPEL_CMD_SETLIGHT	0x02

#define AMPEL_VAL_BLINK		0x8
#define AMPEL_VAL_RED		0x1
#define AMPEL_VAL_GREEN		0x2

void I3C_reset_ampel() {
  I2C_command(I2C_FD_AMPEL, AMPEL_CMD_RESET, 0x0);
}

///// Ampel /////

static struct ampel_state_t AMPEL_OFF = { false, false, false };

static time_t monotonic_now(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec;
}

uint8_t ampel_set_color(struct ampel_state_t color) {
  uint8_t val = 0;
  val |= color.red   ? AMPEL_VAL_RED   : 0;
  val |= color.green ? AMPEL_VAL_GREEN : 0;
  val |= color.blink ? AMPEL_VAL_BLINK : 0;
 
  uint8_t ret;
  ret = I2C_command(I2C_FD_AMPEL,
                    AMPEL_CMD_SETLIGHT, val);

  return ret;
}

/**
 * Apply physical LED output based on current nightmode state.
 * During nightmode the LEDs stay off unless the brief visibility timer is
 * active.  Outside nightmode the requested state is shown as-is.
 */
static void apply_physical_state(void) {
  if (!nightmode_active) {
    ampel_set_color(requested_state);
    return;
  }

  if (nightmode_show_until != 0 && monotonic_now() < nightmode_show_until)
    ampel_set_color(requested_state);
  else
    ampel_set_color(AMPEL_OFF);
}

///// Command events

void mqtt_message_callback(struct mosquitto *mosq,
                           void *obj, 
                          const struct mosquitto_message *message)
{
  if (message->payloadlen)
    syslog(LOG_INFO, "got message '%.*s' for topic '%s'\n", message->payloadlen, (char*) message->payload, message->topic);
  else
    syslog(LOG_INFO, "Got empty message for topic '%s'\n", message->topic);

  bool match_ampel = false;
  mosquitto_topic_matches_sub(MQTT_AMPEL_TOPIC, message->topic, &match_ampel);
  if (match_ampel) {
    const char* command = message->payload;

    struct ampel_state_t state = { .red = false, .green = false, .blink = false };

    if (!command)
    {
      // nop
    } else if (strcmp(command, "red") == 0) {
      state.red = true;
    } else if (strcmp(command, "green") == 0) {
      state.green = true;
    } else if (strcmp(command, "red blink") == 0) {
      state.red = true;
      state.blink = true;
    } else if (strcmp(command, "green blink") == 0) {
      state.green = true;
      state.blink = true;
    }

    requested_state = state;

    if (nightmode_active) {
      /* Show state briefly; (re)start the visibility timer */
      nightmode_show_until = monotonic_now() + NIGHTMODE_VISIBILITY_SECS;
    }

    apply_physical_state();
    return;
  }

  bool match_nightmode = false;
  mosquitto_topic_matches_sub(MQTT_NIGHTMODE_TOPIC, message->topic, &match_nightmode);
  if (match_nightmode) {
    const char* payload = message->payload;
    if (!payload)
      return;

    if (strcmp(payload, "on") == 0) {
      if (!nightmode_active) {
        syslog(LOG_INFO, "Nightmode enabled.");
        nightmode_active = true;
        nightmode_show_until = 0;
        ampel_set_color(AMPEL_OFF);
      }
    } else if (strcmp(payload, "off") == 0) {
      if (nightmode_active) {
        syslog(LOG_INFO, "Nightmode disabled.");
        nightmode_active = false;
        nightmode_show_until = 0;
        ampel_set_color(requested_state);
      }
    }
  }
}


int main(int argc, char *argv[]) {
  service_start("ampel", "Starting Ampel controller.");
  service_setup_signals();

  // initialize I2C
  I2C_init();

  // initialize MQTT
  struct mosquitto *mosq = mqtt_service_init("ampel");

  if (mosq) {
    int ret = mqtt_service_connect(mosq, MQTT_HOST, MQTT_PORT, 30);
    if (ret != MOSQ_ERR_SUCCESS) {
      mqtt_service_cleanup(mosq);
      service_stop("Ampel controller finished.");
      return 1;
    }

    mosquitto_message_callback_set(mosq, mqtt_message_callback);
    mosquitto_subscribe(mosq, NULL, MQTT_AMPEL_TOPIC, 0);
    mosquitto_subscribe(mosq, NULL, MQTT_NIGHTMODE_TOPIC, 0);
  }

  service_notify_ready();

  while (service_is_running()) {
    mqtt_service_loop(mosq, 100);

    /* During nightmode, check if the brief visibility timer has just expired */
    if (nightmode_active && nightmode_show_until != 0
        && monotonic_now() >= nightmode_show_until) {
      nightmode_show_until = 0;
      ampel_set_color(AMPEL_OFF);
    }

    sleep(1);
  }

  mqtt_service_cleanup(mosq);
  service_stop("Ampel controller finished.");
  return 0;
}
