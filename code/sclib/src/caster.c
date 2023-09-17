#include "sclib/caster.h"

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

#define SC_CASTER_DIST_THRESHOLD 5000

// Thread.
#define SC_CASTER_THREAD_STACK_SIZE (8 * 1024)
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

// Dumps both accelerometer and motion detector data in CSV format, with header.
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

// Test.
struct sc_signal signal;
static int process_buffer() {
  // dump_buffer();
  // sc_led_flash(2);
  // k_msleep(2000);
  // __ASSERT_NO_MSG(!sc_accel_reset_fifo());
  // return 0;

  if (mode == MODE_RECORD) {
    LOG_DBG("Will store the signal on slot %d.", slot);
    memcpy(signal.entries, fifo_buffer,
           fifo_buffer_len * sizeof(struct sc_accel_entry));
    signal.len = fifo_buffer_len;
    RET_IF_ERR(sc_ss_store(slot, &signal));
    LOG_DBG("Stored signal.\n");
    sc_led_flash(5);
    k_msleep(1000);
    return 0;
  }

  LOG_DBG("Will compare the signal.");

  for (uint8_t maybe_slot = 0; maybe_slot < SC_SIGNAL_STORE_MAX_SIGNALS;
       maybe_slot++) {
    if (sc_ss_load(/*slot=*/maybe_slot, &signal)) {
      LOG_DBG("Failed to load signal from slot %d", maybe_slot);
      continue;
    }

    // Compare the signal.
    LOG_DBG("Comparing signal. Stored len: %d, query len: %d.\n", signal.len,
            fifo_buffer_len);
    size_t dist = dtw(signal.entries, signal.len, fifo_buffer, fifo_buffer_len);
    LOG_DBG("Distance: %d\n", dist);

    if (dist < SC_CASTER_DIST_THRESHOLD) {
      LOG_DBG("Matched slot %d", maybe_slot);
      sc_led_flash(maybe_slot + 1);
      if (user_callback != NULL) {
        user_callback(maybe_slot);
      }
      goto END;
    } else {
      LOG_DBG("Not matched!");
    }
  }
  sc_led_flash(4);

END:
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
  sc_led_flash(slot + 1);
  fifo_buffer_len = 0;
  k_msleep(1000);
}

static void sc_caster_thread_fn(void *, void *, void *) {
  struct button_event_queue_el msg;
  struct sc_accel_entry entry;

  while (true) {
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
               msg.event == SC_BUTTON_EVENT_LONG_PRESS) {
      LOG_DBG("Switching to replay mode");
      change_mode(&mode, &state, MODE_REPLAY);
    }

    // FIFO is not ready (hopefully).
    if (sc_accel_read(&entry)) {
      continue;
    }

    sc_md_ingest(&md, &entry);

    // We are not capturing.
    if (state == STATE_READY) {
      // if (!sc_md_is_still(&md)) {
      if (!sc_md_is_still(&md)) {
        LOG_DBG("Will start to capture");
        fifo_buffer_len = 0;
        state = STATE_CAPTURING;
      }
    } else if (state == STATE_CAPTURING) {
      if (fifo_buffer_len == SC_SIGNAL_STORE_MAX_SAMPLES) {
        LOG_DBG("Buffer full. Discarding.");
        state = STATE_WAITING;
        fifo_buffer_len = 0;
      } else if (!sc_md_is_still(&md)) {
        fifo_buffer[fifo_buffer_len++] = entry;
      } else {
        LOG_DBG("Stopped capturing. Confirm?");
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
      if (sc_md_is_still(&md)) {
        LOG_DBG("Will get ready");
        state = STATE_READY;
        sc_led_flash(1);
      }
    }
  }
}

int sc_caster_init(sc_caster_callback_t callback) {
  LOG_DBG("Initializing");
  RET_IF_ERR(sc_led_init());
  RET_IF_ERR(sc_button_init());
  RET_IF_ERR(sc_accel_init());
  RET_IF_ERR(sc_ss_init());

  sc_led_flash(1);

  sc_md_init(&md);

  sc_button_register_callback(button_callback);

  user_callback = callback;

  // Set thread name.
  k_tid_t tid =
      k_thread_create(&sc_caster_thread, sc_caster_stack_area,
                      K_THREAD_STACK_SIZEOF(sc_caster_stack_area),
                      sc_caster_thread_fn, /*p1=*/NULL, /*p2=*/NULL,
                      /*p3=*/NULL, SC_CASTER_THREAD_PRIORITY, /*options=*/0,
                      /*delay=*/K_NO_WAIT);
  k_thread_name_set(tid, "sc_caster_thread");
  return 0;
}