# cryo-node

STM32 firmware for a small monitoring node, like the ones used on medical gas lines and cryogenic storage tanks: it reads line pressure from a 4–20 mA transmitter, detects sensor faults, and raises alarms with delay, hysteresis and a silence button.

Hardware: NUCLEO-F446RE (Cortex-M4F, 84 MHz). The pressure transmitter is simulated on the board itself (DAC → jumper → ADC).

## Status (v1)

| Step | What it does | Status |
|---|---|---|
| 0 | Heartbeat LED, project bring-up, memory map checked in the debugger | Done |
| 1 | Debug messages over USART2 with a register-level driver (`my_putc` waits for TXE) | Done |
| 2 | Blue button on EXTI, both edges, 50 ms debounce restarted on every edge | Done |
| 2b | TIM6 1 ms tick replaces `HAL_Delay`; main loop runs on flags | Done |
| 3 | Pressure from a simulated 4–20 mA transmitter: ADC → mA → bar, NE43 fault detection | Done |
| 4 | ADC triggered by TIM3 every 1 ms, circular DMA into a ping-pong buffer, 20-sample average per half | Done |
| 5 | Alarm state machine (pure C, unit tested on the PC), running on the board with LED patterns and button acknowledge | Done |
| — | Modbus RTU slave (CRC16, 3.5-character timeout), Python test master, CI | Next (v2) |

## How it works

```text
TIM3 (1 kHz TRGO) --> ADC1 IN0 --> DMA2 Stream0 (circular, 40 samples)
                                     |-- half full  --> average samples 0..19
                                     '-- full       --> average samples 20..39
                                   20 samples = 20 ms = one 50 Hz mains cycle

TIM6 (1 ms) --> tick flags
main loop:
  every 100 ms : alarm_update(mA)  +  LED pattern
  every 1 s    : step the simulated sensor, print one status line
  button press : EXTI sets ack_request, main loop calls alarm_acknowledge()
```

The interrupts only set flags or copy data. All decisions are made in the main loop.

### Alarm rules

The logic lives in [`core/`](core) and has no HAL calls, so the same code runs on the STM32 and in the PC tests.

| Condition | Result |
|---|---|
| Loop current < 3.6 mA or > 21 mA | `SENSOR_FAULT` (NAMUR NE43) |
| Pressure < 3.2 bar | `LOW`, clears only above 3.4 bar (hysteresis) |
| Pressure > 4.8 bar | `HIGH`, clears only below 4.6 bar (hysteresis) |
| Any state change | must hold 2 s without a break, so short dips are ignored |
| Blue button | silences the buzzer only; the state stays until the pressure recovers; a new alarm turns the buzzer on again |

LED LD2 shows the state:

| State | LED |
|---|---|
| `NORMAL` | slow blink (500 ms) |
| `LOW` / `HIGH` | fast blink (100 ms) |
| `SENSOR_FAULT` | medium blink (200 ms) |
| any alarm, acknowledged | steady on |

### Simulated 4–20 mA transmitter

A real transmitter drives a current through a 150 Ω shunt, so 4–20 mA becomes 0.6–3.0 V on the ADC pin. Here the board's own DAC produces that voltage and a jumper wire carries it to the ADC.

| Signal | Pin | Arduino header |
|---|---|---|
| DAC OUT1 (simulated sensor) | PA4 | A2 |
| ADC1 IN0 (firmware input) | PA0 | A0 |

Wire **A2 to A0**. The firmware plays a 35 s scenario in a loop, 5 s per step: 4.0 bar → 3.0 → 3.3 → 3.6 → 5.2 → 4.0 → wire broken.

```text
volt = adc * 3.3 / 4095
mA   = volt * 1000 / 150
bar  = (mA - 4) * 10 / 16          (0-10 bar transmitter)
```

## Results on the board

USART2, 115200 8N1, ST-LINK virtual COM port. Recorded on the bench:

```text
t=5946s   0.00 bar  NORMAL       buzzer=0  [wire broken]
t=5948s   0.00 bar  SENSOR_FAULT buzzer=1  [wire broken]        <- fault after 2 s
t=5952s   3.99 bar  NORMAL       buzzer=0  [4.0 bar, normal]
t=5956s   2.99 bar  NORMAL       buzzer=0  [3.0 bar, low]
t=5957s   2.99 bar  LOW          buzzer=1  [3.0 bar, low]
t=5960s   2.99 bar  LOW          buzzer=0  [3.3 bar, ...]       <- button pressed
t=5962s   3.28 bar  LOW          buzzer=0  [3.3 bar, ...]       <- hysteresis holds LOW
```

| Test on the board | Expected | Result |
|---|---|---|
| Press during `LOW` | buzzer 0, state stays `LOW`, LED steady | Pass |
| No press, then `HIGH` arrives | buzzer back to 1 | Pass |
| Press during `SENSOR_FAULT` | buzzer 0, LED steady | Not verified |
| Several fast presses during `NORMAL` | nothing changes; next alarm still sounds | Not verified |

## PC unit tests

Seven tests for the alarm logic: start state, acknowledge, 2 s delay, buzzer re-arm, hysteresis, short dip ignored, open loop = fault.

```powershell
powershell -ExecutionPolicy Bypass -File tests/run_tests.ps1
```

Needs `gcc` on the PATH (tested with MinGW gcc 6.3). Built with `-Wall -Wextra -Werror`.

## Things I measured or found on the bench

- **Contact bounce on release.** Debouncing only falling edges counted some presses twice: the release also bounces, more than 50 ms after the press. Interrupting on both edges and restarting the window on every edge fixed it.
- **Timing with the DWT cycle counter.** `DWT->CYCCNT` grew by about 84,168,000 per loop: 1 s of `HAL_Delay` plus about 2 ms to send one status line at 115200 baud.
- **A floating ADC input lies.** With the jumper removed, PA0 floats and sometimes reads plausible pressures (4.3 bar). On real hardware the 150 Ω shunt pulls the pin to 0 V, so an open loop always reads as a fault.
- **Background sampling lags by one window.** With DMA the average describes the last 20 ms, so the test sequence became: measure → print → step the DAC.
- **A damaged DAC output buffer.** One day the pressure read mirrored (ADC ≈ 4095 − DAC). Step by step: A0 to GND read 0 and A0 to 3V3 read full scale, so the ADC was fine. Reading PA4 through ADC channel 4 gave about 0 V while the DAC registers (`CR`, `DOR1`) and the pin mode were correct. Turning the output buffer off live in the debugger (`DAC->CR` BOFF1) made the voltage appear. Fix: output buffer disabled in CubeMX, and ADC sampling time raised to 480 cycles, because the unbuffered DAC has about 15 kΩ output impedance.

## Build and flash

1. Open the `firmware/` folder in STM32CubeIDE 1.19 (File → Open Projects from File System).
2. Build: Project → Build Project. `core/` is linked into the project as the folder `cryo_core`.
3. Flash and run: Run (green arrow). Debug: the bug icon.
4. Open a serial terminal on the ST-LINK COM port at 115200 8N1.

## Layout

| Folder | What goes there |
|---|---|
| `firmware/` | STM32CubeIDE project (HAL, drivers, `main.c`) |
| `core/` | Pure C logic with no HAL calls (alarm state machine), tested on the PC |
| `tests/` | PC unit tests for `core/` and the script that runs them |
| `tools/` | Python scripts that talk to the board (v2: Modbus master) |
| `docs/` | Notes, wiring, measurements |

## Next (v2)

- Modbus RTU slave over UART with RS-485 direction control, CRC16 and a 3.5-character frame timeout, robust against partial frames and bad CRC.
- Python Modbus master as an on-board integration test.
- GitHub Actions running the PC unit tests on every push.
