#include "alarm.h"

void alarm_init(alarm_t *a) {
	/* TODO: state NORMAL, pending NORMAL, pending_ms 0, buzzer off. */
	a->state = ALARM_NORMAL;
	a->pending = ALARM_NORMAL;
	a->pending_ms = 0;
	a->buzzer = false;
}

void alarm_update(alarm_t *a, float loop_ma, uint32_t dt_ms) {
	/*
	 * TODO, in three steps:
	 *
	 * 1. Decide where the input points ("target"):
	 *    - fault if loop_ma is outside 3.6 .. 21 mA
	 *    - otherwise convert to bar and compare with the thresholds.
	 *      Use the CURRENT state for hysteresis: if we are in LOW, we stay
	 *      LOW until the pressure is above ALARM_LOW_OFF_BAR, and so on.
	 *
	 * 2. Delay:
	 *    - if target == a->state: nothing to change, clear the pending timer.
	 *    - if target is new (different from a->pending): start counting again.
	 *    - otherwise add dt_ms; when it reaches ALARM_DELAY_MS, accept the change.
	 *
	 * 3. When a change is accepted: turn the buzzer on if the new state
	 *    is not NORMAL, off if it is NORMAL.
	 */
	(void) a;
	(void) loop_ma;
	(void) dt_ms;
}

void alarm_acknowledge(alarm_t *a) {
	/* TODO: silence the buzzer. Do not touch the state. */
	a->buzzer = false;

}
