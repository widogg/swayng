#!/bin/bash
# Assert-based check of the hi-res wheel notch counter (no compositor needed).
# Usage: contrib/check-wheel-notches.sh
set -eu

include_dir=$(realpath "$(dirname "$0")/../include")
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

cc -std=c11 -Wall -Wextra -Werror -I"$include_dir" -o "$tmp/check" -x c - <<'EOF'
#include <assert.h>
#include "sway/input/wheel.h"

// Same clock the axis events carry, advanced by hand.
static uint32_t now = 1000;

static int steps(struct wheel_notch_counter *counter, int32_t delta,
    int idle_msec) {
  return wheel_notch_steps(counter, delta, now, idle_msec);
}

int main(void) {
  struct wheel_notch_counter counter = {.time_msec = now};

  // A legacy wheel reports a whole notch per event.
  assert(!wheel_is_hires(120) && !wheel_is_hires(-240) && !wheel_is_hires(0));
  assert(wheel_is_hires(15) && wheel_is_hires(-112));

  assert(steps(&counter, 120, 150) == 1 && counter.accum == 0);
  assert(steps(&counter, -120, 150) == -1 && counter.accum == 0);

  // Hi-res wheel: 8 events of 15 make exactly one notch, on the last one.
  for (int i = 0; i < 7; i++) {
    assert(steps(&counter, 15, 150) == 0);
  }
  assert(steps(&counter, 15, 150) == 1 && counter.accum == 0);

  // A fast spin reported as one big event fires the binding twice.
  assert(steps(&counter, 250, 150) == 2 && counter.accum == 10);

  // Reversing discards the partial notch; the first full notch back fires.
  assert(steps(&counter, -60, 150) == 0 && counter.accum == -60);
  assert(steps(&counter, -60, 150) == -1 && counter.accum == 0);

  // A zero-delta wheel event keeps the partial notch.
  assert(steps(&counter, -30, 150) == 0);
  assert(steps(&counter, 0, 150) == 0 && counter.accum == -30);

  // Idling past the timeout drops the partial notch.
  now += 150;
  assert(steps(&counter, -30, 150) == 0 && counter.accum == -30);

  // Inside the timeout it carries over, as do fractions with the reset off.
  now += 149;
  assert(steps(&counter, -30, 150) == 0 && counter.accum == -60);
  now += 10000;
  assert(steps(&counter, -60, 0) == -1 && counter.accum == 0);

  return 0;
}
EOF

"$tmp/check"
echo "wheel notch counter OK"
