#include <stdint.h>
#include <stdbool.h>

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include <syslog.h>

#include "../common/i2c_command.h"

#include <mosquitto.h>

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
    printf("got message '%.*s' for topic '%s'\n", message->payloadlen, (char*) message->payload, message->topic);
  else
    printf("Got empty message for topic '%s'\n", message->topic);

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
  // initialize the system logging
  openlog("ampel", LOG_CONS | LOG_PID, LOG_USER);
  syslog(LOG_INFO, "Starting Ampel controller.");

  // initialize I2C
  I2C_init();
  
  // initialize MQTT
  mosquitto_lib_init();
  
  void *mqtt_obj;
  struct mosquitto *mosq;
  mosq = mosquitto_new("ampel", true, mqtt_obj);
  if (((int)mosq == ENOMEM) || ((int)mosq == EINVAL)) {
        syslog(LOG_ERR, "MQTT error %d (%s)!", 
                        (int)mosq,
                        mosquitto_strerror((int)mosq));
    mosq = NULL;
  }
  
  if (mosq) {
    int ret;
    
    ret = mosquitto_connect(mosq, MQTT_HOST, MQTT_PORT, 30);
    if (ret == MOSQ_ERR_SUCCESS)
      syslog(LOG_INFO, "MQTT connection to %s established.", MQTT_HOST);
    else {
        syslog(LOG_ERR, "MQTT error %d (%s)!", 
                        ret,
                        mosquitto_strerror(ret));
        //TODO cleanup
        return -1;
    }      
  }
  
  // subscribe to Ampel topic
  if (mosq) {
    mosquitto_message_callback_set(mosq, mqtt_message_callback);
    mosquitto_subscribe(mosq, NULL, MQTT_AMPEL_TOPIC, 0);
  }

  char run=1;
  int i=0;
  while(run) {
    // call the mosquitto loop to process messages
    if (mosq) {
      int ret;
      ret = mosquitto_loop(mosq, 100, 1);
      // if failed, try to reconnect
      if (ret) {
        syslog(LOG_ERR, "MQTT reconnect.");
        mosquitto_reconnect(mosq);
      }
    }
    
    if (sleep(1)) 
      break;
  }

  // clean-up MQTT
  if (mosq) {
    mosquitto_disconnect(mosq);
    mosquitto_destroy(mosq);
  }
  mosquitto_lib_cleanup();

  syslog(LOG_INFO, "Ampel controller finished.");
  closelog();
    
  return 0;
}
