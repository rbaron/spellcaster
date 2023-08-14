#include "sclib/caster.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "sclib/accel.h"
#include "sclib/button.h"
#include "sclib/dtw.h"
#include "sclib/led.h"
#include "sclib/macros.h"
#include "sclib/motion_detector.h"

// Thread.
#define SC_CASTER_THREAD_STACK_SIZE (3 * 2048)
#define SC_CASTER_THREAD_PRIORITY 5

// We store max 3 seconds of data.
#define SC_CASTER_MAX_SAMPLES (3 * SC_ACCEL_SAMPLE_RATE_HZ)

// Logger.
// LOG_MODULE_REGISTER(sc_caster, CONFIG_SCLIB_LOG_LEVEL);
LOG_MODULE_REGISTER(sc_caster, LOG_LEVEL_DBG);

enum State {
  STATE_READY = 0,
  STATE_CAPTURING = 1,
  STATE_CONFIRMING = 2,
  STATE_WAITING = 3,
};

static enum State state = STATE_WAITING;

static size_t fifo_buffer_len = 0;
static struct sc_accel_entry fifo_buffer[SC_CASTER_MAX_SAMPLES] = {0};

static bool has_saved_signal = false;
static size_t saved_signal_len = 0;
static struct sc_accel_entry saved_signal[SC_CASTER_MAX_SAMPLES] = {0};

// Motion detector.
static struct sc_motion_detector md;

// Thread context.
K_THREAD_STACK_DEFINE(sc_caster_stack_area, SC_CASTER_THREAD_STACK_SIZE);
static struct k_thread sc_caster_thread;

static void process_buffer() {
  // DTW test.
  if (!has_saved_signal) {
    // Save the signal.
    // TODO: memcpy.
    for (size_t i = 0; i < fifo_buffer_len; i++) {
      saved_signal[i] = fifo_buffer[i];
    }
    // memcpy(saved_signal, fifo_buffer,
    //        fifo_buffer_len * sizeof(struct sc_accel_entry));
    saved_signal_len = fifo_buffer_len;
    has_saved_signal = true;
    LOG_DBG("Saved signal.\n");
    sc_led_flash(3);
    return;
  } else {
    // Compare the signal.
    LOG_DBG("Comparing signal. Stored len: %d, query len: %d.\n",
            saved_signal_len, fifo_buffer_len);
    size_t dist =
        dtw(saved_signal, saved_signal_len, fifo_buffer, fifo_buffer_len);
    LOG_DBG("Distance: %d\n", dist);
    sc_led_flash(2);
    return;
  }
}

static void sc_caster_thread_fn(void *, void *, void *) {
  struct sc_accel_entry entry;
  while (true) {
    // FIFO is not ready (hopefully).
    if (sc_accel_read(&entry)) {
      continue;
    }
    // LOG_DBG("%10d %10d %10d %10d %10d %10d", entry.ax, entry.ay, entry.az,
    //         entry.gx, entry.gy, entry.gz);
    // continue;

    sc_md_ingest(&md, &entry);

    // We are not capturing.
    if (state == STATE_READY) {
      if (!sc_md_is_still(&md)) {
        LOG_DBG("Will start to capture");
        fifo_buffer_len = 0;
        state = STATE_CAPTURING;
      }
    } else if (state == STATE_CAPTURING) {
      if (fifo_buffer_len == SC_CASTER_MAX_SAMPLES) {
        LOG_DBG("Buffer full. Confirm?");
        state = STATE_CONFIRMING;
      } else if (!sc_md_is_still(&md)) {
        fifo_buffer[fifo_buffer_len++] = entry;
      } else {
        LOG_DBG("Stopped capturing. Confirm?");
        state = STATE_CONFIRMING;
      }
    } else if (state == STATE_CONFIRMING) {
      // If OK is pressed.
      // if (sc_button_poll(SC_BUTTON_SW1)) {
      // LOG_DBG("OK pressed");
      process_buffer();
      fifo_buffer_len = 0;
      state = STATE_WAITING;
      // }
      // // If cancel is pressed.
      // if (digitalRead(INPUT_CANCEL_PIN) == LOW) {
      //   LOG_DBG("Cancel pressed");
      //   fifo_buffer_len = 0;
      //   state = STATE_WAITING;
      // }
    } else if (state == STATE_WAITING) {
      if (sc_md_is_still(&md)) {
        LOG_DBG("Will get ready");
        state = STATE_READY;
        sc_led_flash(1);
      }
    }
  }
}

int sc_caster_init(void) {
  LOG_DBG("Initializing");
  RET_IF_ERR(sc_button_init());
  RET_IF_ERR(sc_accel_init());
  sc_md_init(&md);
  k_thread_create(&sc_caster_thread, sc_caster_stack_area,
                  K_THREAD_STACK_SIZEOF(sc_caster_stack_area),
                  sc_caster_thread_fn, /*p1=*/NULL, /*p2=*/NULL, /*p3=*/NULL,
                  SC_CASTER_THREAD_PRIORITY, /*options=*/0,
                  /*delay=*/K_NO_WAIT);
  return 0;
}