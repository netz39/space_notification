# Raspberry Pi Services

This directory contains the `ledcontrol` and `statusswitch` services for the Netz39 space notification system.

## Dependencies

### Build

| Package | Purpose |
|---|---|
| `gcc` | C compiler |
| `make` | Build system |
| `libmosquitto-dev` | MQTT client library (headers + static lib) |
| `libsystemd-dev` | systemd notification API (headers + static lib) |
| `wiringpi` | GPIO/I²C access on Raspberry Pi (production build only) |

Install on Debian/Raspberry Pi OS:

```sh
# Development build (no hardware required)
sudo apt install gcc make libmosquitto-dev libsystemd-dev

# Production build (Raspberry Pi with hardware)
sudo apt install gcc make libmosquitto-dev libsystemd-dev wiringpi
```

### Runtime

| Package | Purpose |
|---|---|
| `libmosquitto1` | MQTT client runtime library |
| `libsystemd0` | systemd runtime library (pre-installed on systemd systems) |
| `wiringpi` | GPIO/I²C runtime (production only) |

## Building

```sh
# Development build (uses I²C stub, no hardware required)
make dev

# Production build (requires wiringPi and Raspberry Pi hardware)
make
```

## Installation

Copy the binaries to `/usr/local/bin` and install the systemd unit files:

```sh
sudo cp ledcontrol/ledcontrol statusswitch/statusswitch /usr/local/bin/
sudo cp ledcontrol.service statusswitch.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable --now ledcontrol statusswitch
```
