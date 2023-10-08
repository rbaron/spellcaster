#ifndef _MOTION_DETECTOR_H_
#define _MOTION_DETECTOR_H_

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/kernel.h>

#include "sclib/accel.h"

#define SC_MD_HORIZ_TIMER_PERIOD_MS 500

// All fields should be considered private.
struct sc_motion_detector {
  struct k_work_delayable horiz_timer;
  struct k_work_delayable inact_timer;
  struct sc_accel_entry prev_entry;
  bool is_horizontal;
  bool is_inactive;
};

void sc_md_init(struct sc_motion_detector *md);

void sc_md_ingest(struct sc_motion_detector *md,
                  const struct sc_accel_entry *entry);

bool sc_md_is_horizontal(const struct sc_motion_detector *md);

bool sc_md_is_inactive(const struct sc_motion_detector *md);

#endif  // _MOTION_DETECTOR_H_