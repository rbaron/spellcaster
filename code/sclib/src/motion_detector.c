#include "sclib/motion_detector.h"

#include <stdbool.h>
#include <zephyr/logging/log.h>

#define MD_TIMER_PERIOD_MS 500

// Full motion is 2g, in 16 bits, that's sort of +/- 32768.
// #define MD_ACCEL_DIFF_THRESHOLD 1024
#define MD_ACCEL_DIFF_THRESHOLD 2048
#define MD_STILL_Y_ACCEL_THRESHOLD 2048

LOG_MODULE_REGISTER(motion_detector, CONFIG_SCLIB_LOG_LEVEL);
// LOG_MODULE_REGISTER(motion_detector, LOG_LEVEL_DBG);

static uint16_t abs(int16_t x) {
  return x < 0 ? -x : x;
}

static void timer_callback(struct k_work *work) {
  struct k_work_delayable *delayable =
      CONTAINER_OF(work, struct k_work_delayable, work);
  struct sc_motion_detector *md =
      CONTAINER_OF(delayable, struct sc_motion_detector, timer);
  LOG_DBG("Still");
  md->is_still = true;
}

void sc_md_init(struct sc_motion_detector *md) {
  k_work_init_delayable(&md->timer, timer_callback);
  md->is_still = false;
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

  // If any of the axes are large, reset the timer.
  bool is_still_now = abs(diff.ax) < MD_ACCEL_DIFF_THRESHOLD &&
                      abs(diff.ay) < MD_ACCEL_DIFF_THRESHOLD &&
                      abs(diff.az) < MD_ACCEL_DIFF_THRESHOLD &&
                      abs(diff.gx) < MD_ACCEL_DIFF_THRESHOLD &&
                      abs(diff.gy) < MD_ACCEL_DIFF_THRESHOLD &&
                      abs(diff.gz) < MD_ACCEL_DIFF_THRESHOLD;

  bool is_still_horiz_now =
      is_still_now && abs(entry->ay) < MD_STILL_Y_ACCEL_THRESHOLD;

  // Reset the timer if we're not still horizontal.
  if (!is_still_horiz_now) {
    // Restart timer.
    k_work_reschedule(&md->timer, K_MSEC(MD_TIMER_PERIOD_MS));

    // Trigger state transition if we were still.
    if (md->is_still) {
      LOG_DBG("Moving");
      md->is_still = false;
    }
  }

  // Save the current entry.
  md->prev_entry = *entry;
}

bool sc_md_is_still(const struct sc_motion_detector *md) {
  return md->is_still;
}