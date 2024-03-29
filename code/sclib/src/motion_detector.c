#include "sclib/motion_detector.h"

#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <zephyr/logging/log.h>

// Full motion is 4g, in 16 bits
#define MD_ACCEL_DIFF_THRESHOLD (3 * 1024)
#define MD_STILL_Y_ACCEL_THRESHOLD (4 * 1024)

// LOG_MODULE_REGISTER(motion_detector, CONFIG_SCLIB_LOG_LEVEL);
LOG_MODULE_REGISTER(motion_detector, LOG_LEVEL_DBG);

// static uint16_t abs(int16_t x) {
//   return x < 0 ? -x : x;
// }

static void inact_timer_callback(struct k_work *work) {
  struct k_work_delayable *delayable =
      CONTAINER_OF(work, struct k_work_delayable, work);
  struct sc_motion_detector *md =
      CONTAINER_OF(delayable, struct sc_motion_detector, inact_timer);
  // LOG_DBG("Inactive");
  md->is_inactive = true;

  // It it horizontal?
  md->is_horizontal = abs(md->prev_entry.ay) < MD_ACCEL_DIFF_THRESHOLD;

  if (md->is_horizontal) {
    // LOG_DBG("Horizontal");
    md->initial_row_angle = atan2(-1 * md->prev_entry.ax, md->prev_entry.az);
  } else {
    // LOG_DBG("Not horizontal");
  }

  // Logs all entries.
  // LOG_DBG("Entry: %d, %d, %d, %d, %d, %d", md->prev_entry.ax,
  // md->prev_entry.ay,
  //         md->prev_entry.az, md->prev_entry.gx, md->prev_entry.gy,
  //         md->prev_entry.gz);

  LOG_DBG("Timer cb. inactive %d, horizontal: %d", md->is_inactive,
          md->is_horizontal);

  // Reset the timer if we're not still horizontal.
  if (!md->is_inactive || !md->is_horizontal) {
    k_work_reschedule(&md->inact_timer, K_MSEC(SC_MD_INACTIVE_TIMER_PERIOD_MS));
  }
}

void sc_md_init(struct sc_motion_detector *md) {
  k_work_init_delayable(&md->inact_timer, inact_timer_callback);
  md->is_horizontal = false;
  md->is_inactive = false;
}

void sc_md_ingest(struct sc_motion_detector *md,
                  const struct sc_accel_entry *entry) {
  // Calculate the difference between the current and previous acceleration.
  struct sc_accel_entry diff;
  diff.ax = entry->ax - md->prev_entry.ax;
  diff.ay = entry->ay - md->prev_entry.ay;
  diff.az = entry->az - md->prev_entry.az;
  diff.gx = entry->gx - md->prev_entry.gx;
  diff.gy = entry->gy - md->prev_entry.gy;
  diff.gz = entry->gz - md->prev_entry.gz;

  // TODO: A better way of detecting stillness. Now we're only looking at
  // acceleration changes, which is wrong -- a constant acceleration does not
  // mean stillness. Ideas:
  // 1. Doubly integrate accelerations, and account for gravity.
  // 2. Check that gravity is the only acceleration.
  // 3. See if the gyroscope gives us this info.

  // If any of the axes are large, reset the timer.
  bool is_still_now = abs(diff.ax) < MD_ACCEL_DIFF_THRESHOLD &&
                      abs(diff.ay) < MD_ACCEL_DIFF_THRESHOLD &&
                      abs(diff.az) < MD_ACCEL_DIFF_THRESHOLD &&
                      abs(diff.gx) < MD_ACCEL_DIFF_THRESHOLD &&
                      abs(diff.gy) < MD_ACCEL_DIFF_THRESHOLD &&
                      abs(diff.gz) < MD_ACCEL_DIFF_THRESHOLD;

  if (!is_still_now) {
    // Restart timer.
    k_work_reschedule(&md->inact_timer, K_MSEC(SC_MD_INACTIVE_TIMER_PERIOD_MS));
    // Trigger state transition if we were still.
    if (md->is_inactive) {
      LOG_DBG("Entered active state");
      // LOG_DBG("Diff: %d, %d, %d, %d, %d, %d", diff.ax, diff.ay, diff.az,
      //         diff.gx, diff.gy, diff.gz);
      md->is_inactive = false;
      md->is_horizontal = false;
    }
  }

  // Save the current entry.
  md->prev_entry = *entry;
}

bool sc_md_is_horizontal(const struct sc_motion_detector *md) {
  return md->is_inactive && md->is_horizontal;
}

bool sc_md_is_inactive(const struct sc_motion_detector *md) {
  return md->is_inactive;
}

float sc_md_initial_row_angle(const struct sc_motion_detector *md) {
  return md->initial_row_angle;
}
