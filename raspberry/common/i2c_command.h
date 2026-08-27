#ifndef I2C_COMMAND_H
#define I2C_COMMAND_H

#include "i2c_hal.h"

#include <stdlib.h>
#include <errno.h>
#include <syslog.h>

#define I2C_ERR_INVALIDARGUMENT -2

union I2C_result {
  unsigned char c[2];
  unsigned short r;
};

/**
  * Initialize an I2C channel to the specified address. Exits with an error
  * message if the initialization fails,
  *
  * @param addr The target address.
  * @return File Descriptor for the I2C channel
  */
static int I2C_setup_fd(const int addr) {
  const int fd = i2c_hal_setup(addr);
  if (!fd) {
    syslog(LOG_EMERG, "Error %d on I2C initialization!", errno);
    exit(-1);
  }
  return fd;
}

static int I2C_command(const int fd, const char command, const char data) {
  // check parameter range
  if ((command < 0) || (command > 0x07))
    return I2C_ERR_INVALIDARGUMENT;
  if ((data < 0) || (data > 0x0f))
    return I2C_ERR_INVALIDARGUMENT;

  // build the I2C data byte
  // arguments have been checked,
  // this cannot be negative or more than 8 bits
  unsigned char send = (command << 4) + data;

  // calculate the parity
  char v = send;
  char c;
  for (c = 0; v; c++)
    v &= v-1;
  c &= 1;

  // set parity bit
  send += (c << 7);

  union I2C_result result;
  result.r = 0;

  // maximal number of tries
  int hops=20;

  // try for hops times until the result is not zero
  while (!result.c[0] && --hops) {
    // send command
    result.r = i2c_hal_read_reg16(fd, send);

    // check for transmission errors: 2nd byte is inverted 1st byte
    const unsigned char check = ~result.c[0];
    if (result.c[1] != check)
      // if no match, reset the result
      result.r = 0;
  }

  if (!hops)
    syslog(LOG_DEBUG, "Giving up transmission!\n");

  return result.c[0];
}

#endif /* I2C_COMMAND_H */
