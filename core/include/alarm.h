/*
 * Pressure alarm logic for a medical gas line (4-20 mA transmitter, 0-10 bar).
 *
 * Pure C, no HAL: it runs the same on the STM32 and in the PC unit tests.
 *
 * Rules (agreed design):
 *   - Loop current below 3.6 mA or above 21 mA  -> SENSOR_FAULT (NAMUR NE43).
 *   - Pressure below 3.2 bar                     -> LOW alarm.
 *     LOW clears only above 3.4 bar               (hysteresis).
 *   - Pressure above 4.8 bar                     -> HIGH alarm.
 *     HIGH clears only below 4.6 bar              (hysteresis).
 *   - Every state change, in both directions, must hold for ALARM_DELAY_MS
 *     without interruption before it is accepted.
 *   - The buzzer turns on when the state changes to anything other than NORMAL.
 *   - Acknowledge (blue button) silences the buzzer only. The state stays
 *     until the pressure itself recovers. A new alarm turns the buzzer on again.
 */
#ifndef ALARM_H
#define ALARM_H

#include <stdbool.h>
#include <stdint.h>

#define ALARM_FAULT_LOW_MA   3.6f
#define ALARM_FAULT_HIGH_MA  21.0f

#define ALARM_LOW_ON_BAR     3.2f
#define ALARM_LOW_OFF_BAR    3.4f
#define ALARM_HIGH_ON_BAR    4.8f
#define ALARM_HIGH_OFF_BAR   4.6f

#define ALARM_DELAY_MS       2000u

typedef enum {
	ALARM_NORMAL = 0,
	ALARM_LOW,
	ALARM_HIGH,
	ALARM_SENSOR_FAULT
} alarm_state_t;

typedef struct {
	alarm_state_t state;     /* the state the device reports */
	alarm_state_t pending;   /* the state the input points to right now */
	uint32_t pending_ms;     /* how long 'pending' has held without a break */
	bool buzzer;             /* true = sound on */
} alarm_t;

/* Start in NORMAL, buzzer off, nothing pending. */
void alarm_init(alarm_t *a);

/* Call every dt_ms with the measured loop current in mA. */
void alarm_update(alarm_t *a, float loop_ma, uint32_t dt_ms);

/* The nurse pressed the button: silence the buzzer, keep the state. */
void alarm_acknowledge(alarm_t *a);

#endif /* ALARM_H */
