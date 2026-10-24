#include "alarm.h"

void alarm_init(alarm_t *a)
{
	a->state = ALARM_NORMAL;
	a->pending = ALARM_NORMAL;
	a->pending_ms = 0;
	a->buzzer = false;
}

void alarm_update(alarm_t *a, float loop_ma, uint32_t dt_ms)
{
	/* 1. Where does the input point? */
	alarm_state_t target;

	if (loop_ma < ALARM_FAULT_LOW_MA || loop_ma > ALARM_FAULT_HIGH_MA)
	{
		target = ALARM_SENSOR_FAULT;
	}
	else
	{
		float bar = (loop_ma - 4.0f) * 10.0f / 16.0f;
		if (bar < ALARM_LOW_ON_BAR)
		{
			target = ALARM_LOW;
		}
		else if (bar > ALARM_HIGH_ON_BAR)
		{
			target = ALARM_HIGH;
		}
		else
		{
			target = ALARM_NORMAL;
		}
	}

	/* 2. Delay: the change must hold for ALARM_DELAY_MS. */
	if (target == a->state)
	{
		a->pending_ms = 0;
	}
	else if (target != a->pending)
	{
		a->pending = target;
		a->pending_ms = dt_ms; /* the first step already counts */
	}
	else
	{
		a->pending_ms += dt_ms;
		if (a->pending_ms >= ALARM_DELAY_MS)
		{
			a->state = target;
			a->pending_ms = 0;
			a->buzzer = (target != ALARM_NORMAL);
		}
	}
}

void alarm_acknowledge(alarm_t *a)
{
	a->buzzer = false;
}