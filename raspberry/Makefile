# http://stackoverflow.com/questions/2145590/what-is-the-purpose-of-phony-in-a-makefile

CC      = gcc
DEBUG   = -O3
INCLUDE = -I/usr/local/include
CFLAGS  = $(DEBUG) -Wall $(INCLUDE) -Winline -pipe

LDFLAGS = -L/usr/local/lib

# Set USE_STUB=1 for a development build without wiringPi or physical hardware.
# Default (USE_STUB=0) produces a production build for Raspberry Pi.
USE_STUB ?= 0

ifeq ($(USE_STUB), 1)
  I2C_HAL_OBJ = common/i2c_hal_stub.o
  LDLIBS      = -lpthread -lm -lmosquitto
else
  I2C_HAL_OBJ = common/i2c_hal_wiringpi.o
  LDLIBS      = -lwiringPi -lwiringPiDev -lpthread -lm -lmosquitto
endif

TARGETS  = ledcontrol statusswitch
BINARIES = $(foreach target,$(TARGETS),$(target)/$(target))
OBJECTS  = $(addsuffix .o,$(BINARIES))

.PHONY: all dev clean

all: $(BINARIES)

dev:
	$(MAKE) USE_STUB=1

$(BINARIES): %: %.o $(I2C_HAL_OBJ)
	$(CC) -o $@ $^ $(LDFLAGS) $(LDLIBS)

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -f $(OBJECTS) $(BINARIES) $(I2C_HAL_OBJ)
