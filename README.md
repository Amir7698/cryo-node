# cryo-node

STM32 firmware for a small monitoring node, like the ones used on medical gas lines and cryogenic storage tanks: it reads line pressure, raises alarms, and reports over Modbus.

Hardware: NUCLEO-F446RE (Cortex-M4F, 84 MHz). Parts I don't have yet are simulated on the board itself.

> Work in progress. The status table below says what runs today and what comes next.

## Status

| Step | What it does | Status |
|---|---|---|
| 0 | Heartbeat LED, project bring-up, memory map checked in the debugger | Done |
| 1 | Debug messages over USART2 with a register-level driver (`my_putc` waits for TXE) | Done |
| 2 | Blue button on EXTI, both edges, 50 ms debounce restarted on every edge | Done |
| 2b | TIM6 1 ms tick replaces `HAL_Delay`; main loop runs on flags | Done |
| 3 | Pressure from a simulated 4–20 mA transmitter: ADC → mA → bar, NE43 fault detection | Done |
| 4 | ADC triggered by a timer, samples moved by DMA, 20 ms averaging | Next |
| 5 | Modbus RTU slave over UART, CRC16, 3.5-character frame timeout | Planned |
| 6 | Temperature sensor over I²C | Planned |
| 7 | Alarm state machine with hysteresis and delay | Planned |
| 8 | Host unit tests for the pure C logic, Python Modbus master, CI | Planned |

## How it works today

Every 1 ms TIM6 overflows and its interrupt raises `tick_1ms`. Every 1000 ticks it raises `tick_1s`. The main loop never blocks: on `tick_1s` it toggles the heartbeat LED, steps the simulated sensor, reads the ADC and prints one line on USART2.

```text
TIM6 (PSC 84-1, ARR 1000-1)  --1 ms-->  HAL_TIM_PeriodElapsedCallback  -->  tick_1s
EXTI15_10 (PC13, both edges) ---------> HAL_GPIO_EXTI_Callback        -->  button_count
main loop: if (tick_1s) { LED, DAC step, ADC read, convert, print }
```

### Simulated 4–20 mA transmitter

A real transmitter drives a current through a 150 Ω shunt, so 4–20 mA becomes 0.6–3.0 V on the ADC pin. Without a transmitter, the board's own DAC produces that voltage and a jumper wire carries it to the ADC.

| Signal | Pin | Arduino header |
|---|---|---|
| DAC OUT1 (simulated sensor) | PA4 | A2 |
| ADC1 IN0 (firmware input) | PA0 | A0 |

Wire **A2 to A0**. The DAC steps by 512 counts every second, so the output covers healthy readings and the open-loop region below 4 mA.

Conversion:

```text
volt = adc * 3.3 / 4095
mA   = volt * 1000 / 150
bar  = (mA - 4) * 10 / 16          (0-10 bar transmitter)

mA < 3.6   -> SENSOR FAULT (open loop)     NAMUR NE43
mA > 21.0  -> OVERRANGE
```

Example output on USART2 (115200 8N1, ST-LINK virtual COM port):

```text
adc= 239 SENSOR FAULT (open loop)
adc= 490 SENSOR FAULT (open loop)
adc=1022   5.49 mA   0.93 bar
adc=2047  10.99 mA   4.37 bar
adc=3599  19.33 mA   9.58 bar
```

## Things I measured or found on the bench

- **Contact bounce on release.** Debouncing only falling edges counted some presses twice: the release also bounces, more than 50 ms after the press. Interrupting on both edges and restarting the window on every edge fixed it.
- **Timing with the DWT cycle counter.** `DWT->CYCCNT` grew by about 84,168,000 per loop: 1 s of `HAL_Delay` plus about 2 ms to send one status line at 115200 baud.
- **The buffered DAC cannot reach 0 V.** DAC = 0 reads about 240 counts (≈ 0.2 V). Harmless here, because a 4–20 mA signal never goes below 0.6 V.
- **A floating ADC input lies.** With the jumper removed, PA0 floats and sometimes reads plausible pressures (4.3 bar). On real hardware the 150 Ω shunt pulls the pin to 0 V, so an open loop always reads as a fault. An analog input must never be left floating.

## Build and flash

1. Open the `firmware/` folder in STM32CubeIDE 1.19 (File → Open Projects from File System).
2. Build: Project → Build Project.
3. Flash and run: Run (green arrow). Debug: the bug icon.
4. Open a serial terminal on the ST-LINK COM port at 115200 8N1.

## Layout

| Folder | What goes there |
|---|---|
| `firmware/` | STM32CubeIDE project (HAL, drivers, `main.c`) |
| `core/` | Pure C logic with no HAL calls (CRC, Modbus frames, scaling, alarms), so it can be tested on a PC |
| `tests/` | Unit tests for `core/`, built and run on the PC |
| `tools/` | Python scripts that talk to the board (Modbus master, test runner) |
| `docs/` | Notes, wiring, measurements |
