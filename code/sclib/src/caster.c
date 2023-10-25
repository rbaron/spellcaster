#include "sclib/caster.h"

#include <math.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "sclib/accel.h"
#include "sclib/button.h"
#include "sclib/dtw.h"
#include "sclib/flash_fs.h"
#include "sclib/led.h"
#include "sclib/macros.h"
#include "sclib/motion_detector.h"
#include "sclib/signal_store.h"
#include "sclib/vibration.h"

#define SC_CASTER_DIST_THRESHOLD 8000

// Signals less than 500 ms (after discarding SC_MD_HORIZ_TIMER_PERIOD_MS), are
// discarded.
#define SC_CASTER_MIN_SIGNAL_MS 500

// Thread.
#define SC_CASTER_THREAD_STACK_SIZE (4 * 1024)
#define SC_CASTER_THREAD_PRIORITY 5

// Logger.
// LOG_MODULE_REGISTER(sc_caster, CONFIG_SCLIB_LOG_LEVEL);
LOG_MODULE_REGISTER(sc_caster, LOG_LEVEL_DBG);

enum Mode {
  MODE_REPLAY = 0,
  MODE_RECORD = 1,
};

static enum Mode mode = MODE_REPLAY;
static uint8_t slot = 0;

enum State {
  STATE_READY = 0,
  STATE_CAPTURING = 1,
  STATE_CONFIRMING = 2,
  STATE_WAITING = 3,
};

static enum State state = STATE_WAITING;

static size_t fifo_buffer_len = 0;
static struct sc_accel_entry fifo_buffer[SC_SIGNAL_STORE_MAX_SAMPLES] = {0};

// Motion detector.
static struct sc_motion_detector md;

// Thread context.
K_THREAD_STACK_DEFINE(sc_caster_stack_area, SC_CASTER_THREAD_STACK_SIZE);
static struct k_thread sc_caster_thread;

// User callback.
static sc_caster_callback_t user_callback = NULL;

// Signal callback.
static sc_caster_signal_callback_t user_signal_callback = NULL;

static bool should_sleep = false;
static bool sleeping = false;

// Message queue for piping button events to the caster thread.
struct button_event_queue_el {
  sc_button_t button;
  sc_button_event_t event;
};

K_MSGQ_DEFINE(button_event_msgq, sizeof(struct button_event_queue_el),
              /*max_elements=*/10, /*align=*/4);

static void button_callback(sc_button_t button, sc_button_event_t event) {
  LOG_DBG("Button %d event %d", button, event);

  struct button_event_queue_el msg = {.button = button, .event = event};
  if (k_msgq_put(&button_event_msgq, &msg, K_NO_WAIT)) {
    LOG_WRN("Failed to put button event in queue");
  }
}

// mps both accelerometer and motion detector data in CSV format, with header.
static void dump_buffer() {
  LOG_DBG("\n---BEGIN DUMP---");
  LOG_DBG("ax,ay,az,gx,gy,gz");
  for (size_t i = 0; i < fifo_buffer_len; i++) {
    LOG_DBG("%d,%d,%d,%d,%d,%d", fifo_buffer[i].ax, fifo_buffer[i].ay,
            fifo_buffer[i].az, fifo_buffer[i].gx, fifo_buffer[i].gy,
            fifo_buffer[i].gz);
  }
  LOG_DBG("\n---END DUMP---");
}

// r is the initial row angle (in radians).
static void project_signal_gravity(struct sc_accel_entry *entry, size_t len,
                                   float r) {
  int16_t tmp_az, tmp_gz;
  for (size_t i = 0; i < len; i++) {
    tmp_az = entry[i].az;
    tmp_gz = entry[i].gz;
    entry[i].az = tmp_az * cos(r) - entry[i].ax * sin(r);
    entry[i].ax = tmp_az * sin(r) + entry[i].ax * cos(r);
    entry[i].gz = tmp_gz * cos(r) - entry[i].gx * sin(r);
    entry[i].gx = tmp_gz * sin(r) + entry[i].gx * cos(r);
  }
}

// Test.
struct sc_signal signal;
static int process_buffer() {
  // If less than half a second of data, discard.
  if (fifo_buffer_len <
      (SC_CASTER_MIN_SIGNAL_MS * SC_ACCEL_SAMPLE_RATE_HZ) / 1000) {
    LOG_DBG("Buffer too short. Discarding.");
    goto END;
  }

  if (user_signal_callback != NULL) {
    user_signal_callback(sc_md_initial_row_angle(&md), fifo_buffer,
                         fifo_buffer_len);
  }

  // Project onto the canonical frame of reference (gravity is down).
  project_signal_gravity(fifo_buffer, fifo_buffer_len,
                         sc_md_initial_row_angle(&md));

  if (mode == MODE_RECORD) {
    LOG_DBG("Will store the signal on slot %d.", slot);
    memcpy(signal.entries, fifo_buffer,
           fifo_buffer_len * sizeof(struct sc_accel_entry));
    signal.len = fifo_buffer_len;
    RET_IF_ERR(sc_ss_store(slot, &signal));
    LOG_DBG("Stored signal -- switching to replay mode.\n");
    sc_led_flash(5);
    mode = MODE_REPLAY;
    goto END;
  }

  LOG_DBG("Will compare the signal.");

  size_t min_dist = SIZE_MAX;
  uint8_t min_slot;
  for (uint8_t maybe_slot = 0; maybe_slot < SC_SIGNAL_STORE_MAX_SIGNALS;
       maybe_slot++) {
    if (sc_ss_load(/*slot=*/maybe_slot, &signal)) {
      LOG_DBG("Failed to load signal from slot %d", maybe_slot);
      continue;
    }

    // Compare the signal.
    // TODO: Bug. Sometimes query_len < 0 here.
    LOG_DBG("Comparing signal. Stored len: %d, query len: %d.\n", signal.len,
            fifo_buffer_len);

    size_t dist = dtw(signal.entries, signal.len, fifo_buffer, fifo_buffer_len);
    LOG_DBG("Distance: %d\n", dist);

    if (dist < min_dist) {
      min_dist = dist;
      min_slot = maybe_slot;
    }
  }

  if (min_dist < SC_CASTER_DIST_THRESHOLD) {
    LOG_DBG("Matched slot %d", min_slot);
    sc_vib_yes_async();
    if (user_callback != NULL) {
      user_callback(min_slot);
    }
    goto END;
  } else {
    LOG_DBG("Not matched!");
    // sc_led_flash(1);
    sc_vib_no();
    goto END;
  }

END:
  // May trigger md's horizontal timer?
  k_msleep(2000);
  __ASSERT_NO_MSG(!sc_accel_reset_fifo());
  return 0;
}

int handle_replay_mode(enum State *state, enum Mode *mode,
                       const struct sc_accel_entry *entry) {
  return 0;
}

int handle_record_mode(enum State *state, enum Mode *mode,
                       const struct sc_accel_entry *entry) {
  return 0;
}

static void change_mode(enum Mode *mode, enum State *state,
                        enum Mode new_mode) {
  *mode = new_mode;
  *state = STATE_WAITING;
  // sc_led_flash(slot + 1);
  sc_led_flash(1);
  fifo_buffer_len = 0;

  // Watch out so we don't trigger the motion detector.
  // k_msleep(1000);
}

static void sc_caster_thread_fn(void *, void *, void *) {
  LOG_DBG("Starting caster thread");

  struct button_event_queue_el msg;
  struct sc_accel_entry entry;

  while (true) {
    if (should_sleep) {
      LOG_DBG("Going to sleep.");
      // End thread. A new one will be created when we wake up.
      // sc_accel_sleep();
      sleeping = true;
      return;
    }

    // Get button event from queue.
    if (k_msgq_get(&button_event_msgq, &msg, K_NO_WAIT)) {
      msg.button = SC_BUTTON_NONE;
      msg.event = SC_BUTTON_EVENT_NONE;
    }

    if (msg.button == SC_BUTTON_A && msg.event == SC_BUTTON_EVENT_SHORT_PRESS) {
      LOG_DBG("Switching to capture mode (slot 0)");
      slot = 0;
      change_mode(&mode, &state, MODE_RECORD);
    } else if (msg.button == SC_BUTTON_A &&
               msg.event == SC_BUTTON_EVENT_DOUBLE_PRESS) {
      LOG_DBG("Switching to capture mode (slot 1)");
      slot = 1;
      change_mode(&mode, &state, MODE_RECORD);
    } else if (msg.button == SC_BUTTON_A &&
               msg.event == SC_BUTTON_EVENT_TRIPLE_PRESS) {
      LOG_DBG("Switching to capture mode (slot 2)");
      slot = 2;
      change_mode(&mode, &state, MODE_RECORD);
    } else if (msg.button == SC_BUTTON_A &&
               msg.event == SC_BUTTON_EVENT_QUADRUPLE_PRESS) {
      LOG_DBG("Switching to capture mode (slot 3)");
      slot = 3;
      change_mode(&mode, &state, MODE_RECORD);
    } else if (msg.button == SC_BUTTON_A &&
               msg.event == SC_BUTTON_EVENT_QUINTUPLE_PRESS) {
      LOG_DBG("Switching to capture mode (slot 4)");
      slot = 4;
      change_mode(&mode, &state, MODE_RECORD);
    } else if (msg.button == SC_BUTTON_A &&
               msg.event == SC_BUTTON_EVENT_LONG_PRESS) {
      LOG_DBG("Switching to replay mode");
      change_mode(&mode, &state, MODE_REPLAY);
    }

    // FIFO is not ready (hopefully).
    if (sc_accel_read(&entry)) {
      // k_msleep(10);
      continue;
    }

    sc_md_ingest(&md, &entry);

    // if (sc_md_is_inactive(&md)) {
    //   LOG_DBG("Still -- going to sleep.");
    //   // End thread. A new one will be created when we wake up.
    //   sc_accel_sleep();
    //   return;
    // }
    // We are not capturing.
    if (state == STATE_READY) {
      if (!sc_md_is_horizontal(&md)) {
        LOG_DBG("Will start to capture");
        fifo_buffer_len = 0;
        state = STATE_CAPTURING;
      }
    } else if (state == STATE_CAPTURING) {
      if (fifo_buffer_len == SC_SIGNAL_STORE_MAX_SAMPLES) {
        LOG_DBG("Buffer full. Discarding.");
        state = STATE_WAITING;
        fifo_buffer_len = 0;
      } else if (!sc_md_is_horizontal(&md)) {
        fifo_buffer[fifo_buffer_len++] = entry;
      } else {
        // Remove 500 ms of data.
        int n_remove =
            (SC_MD_HORIZ_TIMER_PERIOD_MS * SC_ACCEL_SAMPLE_RATE_HZ) / 1000;
        // fifo_buffer_len -= 500 / SC_ACCEL_SAMPLE_PERIOD_MS;
        LOG_DBG(
            "Stopped capturing. Got a total of %d samples ~ %.2f s. Removing "
            "%d (~ %.2f s).",
            fifo_buffer_len, (float)fifo_buffer_len / SC_ACCEL_SAMPLE_RATE_HZ,
            n_remove, (float)n_remove / SC_ACCEL_SAMPLE_RATE_HZ);
        fifo_buffer_len -= n_remove;
        state = STATE_CONFIRMING;
      }
    } else if (state == STATE_CONFIRMING) {
      LOG_DBG("Will process buffer");
      if (process_buffer()) {
        LOG_ERR("Failed to process buffer");
      }
      fifo_buffer_len = 0;
      state = STATE_WAITING;
    } else if (state == STATE_WAITING) {
      if (sc_md_is_horizontal(&md)) {
        LOG_DBG("Will get ready");
        state = STATE_READY;
        // sc_led_flash(1);
        sc_vib_ready();
      }
    }
  }
}

// User callback for accel events.
static void accel_evt_handler(enum sc_accel_evt evt) {
  if (evt == SC_ACCEL_WAKEUP_EVT) {
    LOG_DBG("Accel event: wakeup -- will start caster thread.");
    // __ASSERT_NO_MSG(!sc_accel_init());

    // Blocks here sometimes I think (LED stays on). Why? This should not be an
    // ISR...
    // sc_led_flash(1);

    sc_md_init(&md);
    fifo_buffer_len = 0;
    state = STATE_WAITING;

    __ASSERT(sleeping, "Should be sleeping!");

    // Clear fifo.
    __ASSERT_NO_MSG(!sc_accel_reset_fifo());
    sleeping = false;
    should_sleep = false;
    // Kick off caster thread.
    k_tid_t tid = k_thread_create(&sc_caster_thread, sc_caster_stack_area,
                                  K_THREAD_STACK_SIZEOF(sc_caster_stack_area),
                                  sc_caster_thread_fn, /*p1=*/NULL,
                                  /*p2=*/NULL,
                                  /*p3=*/NULL, SC_CASTER_THREAD_PRIORITY,
                                  /*options=*/0,
                                  /*delay=*/K_NO_WAIT);
    k_thread_name_set(tid, "sc_caster_thread");

    // Maybe restart watchdog here?
  } else if (evt == SC_ACCEL_SLEEP_EVT) {
    LOG_DBG("Accel event: should sleep");
    should_sleep = true;

    // Maybe stop watchdog here?
  }
}

int sc_caster_init(sc_caster_callback_t callback) {
  LOG_DBG("Initializing");
  RET_IF_ERR(sc_led_init());
  RET_IF_ERR(sc_vib_init());
  RET_IF_ERR(sc_button_init());
  RET_IF_ERR(sc_accel_init());
  RET_IF_ERR(sc_ss_init());

  sc_accel_set_evt_handler(accel_evt_handler);

  sc_led_flash(1);
  sc_vib_flash(1);

  sc_md_init(&md);

  sc_button_register_callback(button_callback);

  user_callback = callback;

  // Kick off caster thread.
  k_tid_t tid =
      k_thread_create(&sc_caster_thread, sc_caster_stack_area,
                      K_THREAD_STACK_SIZEOF(sc_caster_stack_area),
                      sc_caster_thread_fn, /*p1=*/NULL, /*p2=*/NULL,
                      /*p3=*/NULL, SC_CASTER_THREAD_PRIORITY, /*options=*/0,
                      /*delay=*/K_NO_WAIT);
  k_thread_name_set(tid, "sc_caster_thread");
  return 0;
}

int sc_caster_set_signal_callback(sc_caster_signal_callback_t callback) {
  user_signal_callback = callback;
  return 0;
}