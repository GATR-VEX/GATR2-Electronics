# pico

Firmware for the [control board](../pcb/ControlBoard/README.md):

- [`locaGATR/`](locaGATR/README.md): reads the sensors and answers the Brain
  over RS-485. Builds with `pio run`, flashes with `pio run -t upload`, host
  tests under `tests/`.
