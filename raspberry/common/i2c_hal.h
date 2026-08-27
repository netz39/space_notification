#ifndef I2C_HAL_H
#define I2C_HAL_H

/**
 * Hardware Abstraction Layer for I2C operations.
 *
 * Implementations:
 *   i2c_hal_wiringpi.c  — production build on Raspberry Pi (requires wiringPi)
 *   i2c_hal_stub.c      — development build (no hardware required)
 */

/**
 * Set up an I2C channel to the given address.
 *
 * @param addr  I2C device address
 * @return      file descriptor; on error behaviour is implementation-defined
 *              (wiringPi exits the process)
 */
int i2c_hal_setup(int addr);

/**
 * Read a 16-bit register value over I2C.
 *
 * @param fd   file descriptor returned by i2c_hal_setup
 * @param reg  register / command byte to send
 * @return     16-bit value read from the device
 */
int i2c_hal_read_reg16(int fd, int reg);

#endif /* I2C_HAL_H */
