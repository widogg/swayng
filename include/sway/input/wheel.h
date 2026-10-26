#ifndef _SWAY_INPUT_WHEEL_H
#define _SWAY_INPUT_WHEEL_H

#include <stdbool.h>
#include <stdint.h>

// One notch is 120 in wl_pointer axis_value120 units; a hi-res wheel splits
// it across several events (e.g. 8 of 15).
#define WHEEL_NOTCH 120

struct wheel_notch_counter {
	int32_t accum;       // partial notch travel, in value120 units
	uint32_t time_msec;  // last wheel event, for the idle reset
};

// Only a hi-res wheel reports a fraction of a notch.
static inline bool wheel_is_hires(int32_t delta_discrete) {
	return delta_discrete % WHEEL_NOTCH != 0;
}

/**
 * Adds hi-res wheel fractions up into whole notches, returning the notches
 * this event completed; reversing or idling discards the partial notch.
 */
static inline int wheel_notch_steps(struct wheel_notch_counter *counter,
		int32_t delta_discrete, uint32_t time_msec, int idle_msec) {
	bool idle = idle_msec > 0 &&
		time_msec - counter->time_msec >= (uint32_t)idle_msec;
	bool reversed = delta_discrete != 0 &&
		(counter->accum < 0) != (delta_discrete < 0);
	if (idle || reversed) {
		counter->accum = 0;
	}

	counter->time_msec = time_msec;
	counter->accum += delta_discrete;

	int steps = counter->accum / WHEEL_NOTCH;
	counter->accum -= steps * WHEEL_NOTCH;
	return steps;
}

#endif
