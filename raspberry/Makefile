# http://stackoverflow.com/questions/2145590/what-is-the-purpose-of-phony-in-a-makefile

CC      = gcc
DEBUG   = -O3
INCLUDE = -I/usr/local/include
CFLAGS  = $(DEBUG) -Wall $(INCLUDE) -Winline -pipe

LDFLAGS = -L/usr/local/lib
LDLIBS  = -lwiringPi -lwiringPiDev -lpthread -lm -lmosquitto

TARGETS  = ledcontrol statusswitch
BINARIES = $(foreach target,$(TARGETS),$(target)/$(target))
OBJECTS  = $(addsuffix .o,$(BINARIES))

.PHONY: clean

all: $(BINARIES)

$(BINARIES): %: %.o
	$(CC) -o $@ $^ $(LDFLAGS) $(LDLIBS)

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -f $(OBJECTS) $(BINARIES)
