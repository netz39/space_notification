/**
 * Production I2C HAL implementation using wiringPi.
 * Compiled for Raspberry Pi targets only.
 */

#include "i2c_hal.h"
#include <wiringPiI2C.h>

int i2c_hal_setup(int addr) {
  return wiringPiI2CSetup(addr);
}

int i2c_hal_read_reg16(int fd, int reg) {
  return wiringPiI2CReadReg16(fd, reg);
}
