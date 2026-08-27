/**
 * Stub I2C HAL implementation for development without hardware.
 *
 * i2c_hal_setup returns the address as a stand-in file descriptor.
 * i2c_hal_read_reg16 returns 0 (no device present).
 */

#include "i2c_hal.h"

int i2c_hal_setup(int addr) {
  return addr;
}

int i2c_hal_read_reg16(int fd, int reg) {
  (void)fd;
  (void)reg;
  return 0;
}
