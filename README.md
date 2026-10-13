# cryo-node

STM32 firmware for a small monitoring node, like the ones used for medical gas lines and cryogenic storage tanks.

- Reads line pressure from a 4–20 mA transmitter (ADC + timer + DMA)
- Reads temperature from an I²C sensor
- Raises alarms with hysteresis and delay
- Exposes the data over Modbus RTU

Hardware: NUCLEO-F446RE. Parts I don't have yet are simulated.

> Work in progress.

## Layout

| Folder | What goes there |
|---|---|
| `firmware/` | STM32CubeIDE project (HAL, drivers, `main.c`) |
| `core/` | Pure C logic with no HAL calls (CRC, Modbus frames, scaling, alarms), so it can be tested on a PC |
| `tests/` | Unit tests for `core/`, built and run on the PC |
| `tools/` | Python scripts that talk to the board (Modbus master, test runner) |
| `docs/` | Notes, wiring, measurements |
