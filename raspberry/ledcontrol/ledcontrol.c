#include <stdint.h>
#include <stdbool.h>

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include "../common/i2c_command.h"
#include "../common/mqtt_service.h"
#include "../common/service.h"

#define I2C_ADDR_AMPEL		0x20

const char* MQTT_HOST 		= "platon.n39.eu";
const int   MQTT_PORT 		= 1883;
const char* MQTT_AMPEL_TOPIC	= "Netz39/Things/Ampel/Light";

struct ampel_state_t {
  bool red;
  bool green;
  bool blink;
};

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

///// Command events

void mqtt_message_callback(struct mosquitto *mosq,
                           void *obj, 
                          const struct mosquitto_message *message)
{
  if (message->payloadlen)
    syslog(LOG_INFO, "got message '%.*s' for topic '%s'\n", message->payloadlen, (char*) message->payload, message->topic);
  else
    syslog(LOG_INFO, "Got empty message for topic '%s'\n", message->topic);

  bool match = false;
  mosquitto_topic_matches_sub(MQTT_AMPEL_TOPIC, message->topic, &match);
  if (match) {
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

    // Set the traffic light state
    ampel_set_color(state);
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
  }

  service_notify_ready();

  while (service_is_running()) {
    mqtt_service_loop(mosq, 100);
    sleep(1);
  }

  mqtt_service_cleanup(mosq);
  service_stop("Ampel controller finished.");
  return 0;
}
