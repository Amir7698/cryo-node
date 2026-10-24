/*
 * Unit tests for core/src/alarm.c. They run on the PC, no board needed.
 * Build and run: powershell -ExecutionPolicy Bypass -File tests/run_tests.ps1
 */
#include <stdio.h>

#include "alarm.h"

static int failures = 0;

#define CHECK(cond)                                                \
	do                                                             \
	{                                                              \
		if (!(cond))                                               \
		{                                                          \
			printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); \
			failures++;                                            \
		}                                                          \
	} while (0)

/* Feed the same current for 'ms' milliseconds, in 100 ms steps. */
static void feed(alarm_t *a, float ma, uint32_t ms)
{
	for (uint32_t t = 0; t < ms; t += 100)
	{
		alarm_update(a, ma, 100);
	}
}

/* 4 mA = 0 bar, 20 mA = 10 bar: mA = 4 + bar * 1.6 */
static float bar_to_ma(float bar)
{
	return 4.0f + bar * 1.6f;
}

/* Example test, as a pattern for the others. */
static void test_starts_normal_and_quiet(void)
{
	alarm_t a;
	alarm_init(&a);
	CHECK(a.state == ALARM_NORMAL);
	CHECK(a.buzzer == false);
}
static void test_ack_silences_buzzer(void)
{
	alarm_t a;
	alarm_init(&a);
	a.buzzer = true;
	a.state = ALARM_LOW;

	alarm_acknowledge(&a);

	CHECK(a.buzzer == false);
	CHECK(a.state == ALARM_LOW);
}

/* Low pressure must hold for 2 s before the alarm is accepted. */
static void test_low_needs_two_seconds(void)
{
	alarm_t a;
	alarm_init(&a);

	feed(&a, bar_to_ma(3.0f), 1900);    /* 3.0 bar for 1.9 s */
	CHECK(a.state == ALARM_NORMAL);     /* not yet */

	feed(&a, bar_to_ma(3.0f), 100);     /* 0.1 s more: 2.0 s in total */
	CHECK(a.state == ALARM_LOW);        /* now */
}

/* Alarm turns the buzzer on, ack silences it, a NEW alarm turns it on again. */
static void test_buzzer_rearms_on_new_alarm(void)
{
	alarm_t a;
	alarm_init(&a);

	feed(&a, bar_to_ma(3.0f), 2000);    /* low pressure */
	CHECK(a.state == ALARM_LOW);
	CHECK(a.buzzer == true);

	alarm_acknowledge(&a);              /* nurse presses the button */
	CHECK(a.buzzer == false);
	CHECK(a.state == ALARM_LOW);

	feed(&a, 2.0f, 2000);               /* wire breaks: 2 mA */
	CHECK(a.state == ALARM_SENSOR_FAULT);
	CHECK(a.buzzer == true);            /* new alarm, sound again */
}

/*
 * TODO: write one function per rule in alarm.h, for example:
 *   - 4.0 bar for 10 s stays NORMAL
 *   - 3.0 bar for 1.9 s is still NORMAL, for 2.0 s becomes LOW, buzzer on
 *   - in LOW, 3.3 bar for 10 s stays LOW (hysteresis), 3.5 bar clears it
 *   - a dip to 3.0 bar for 1 s, then back to 4.0 bar, never alarms
 *   - 2.0 mA for 2 s becomes SENSOR_FAULT (not LOW)
 *   - acknowledge silences the buzzer but keeps the state
 *   - after acknowledge, a new alarm turns the buzzer on again
 * Then call each one from main() below.
 */

int main(void)
{
	test_starts_normal_and_quiet();
	test_ack_silences_buzzer();
	test_low_needs_two_seconds();
	test_buzzer_rearms_on_new_alarm();

	if (failures == 0)
	{
		printf("All tests passed\n");
		return 0;
	}
	printf("%d check(s) failed\n", failures);
	return 1;
}
