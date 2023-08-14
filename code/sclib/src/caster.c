#include "sclib/caster.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "sclib/accel.h"
#include "sclib/button.h"
#include "sclib/dtw.h"
#include "sclib/led.h"
#include "sclib/macros.h"
#include "sclib/motion_detector.h"
#include "sclib/signal_store.h"

#define SC_CASTER_DIST_THRESHOLD 3200

// Thread.
#define SC_CASTER_THREAD_STACK_SIZE (8 * 1024)
#define SC_CASTER_THREAD_PRIORITY 5

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

static void process_buffer() {
  struct sc_signal signal;
  if (sc_ss_load(/*slot=*/0, &signal)) {
    memcpy(signal.entries, fifo_buffer,
           fifo_buffer_len * sizeof(struct sc_accel_entry));
    signal.len = fifo_buffer_len;
    __ASSERT(!sc_ss_store(0, &signal), "Failed to store signal.");
    LOG_DBG("Saved signal.\n");
    sc_led_flash(5);
    // LOG_DBG("WOULD STORE SIGNAL");
    return;
  }

  // LOG_DBG("WOULD COMPARE SIGNAL");

  // Compare the signal.
  LOG_DBG("Comparing signal. Stored len: %d, query len: %d.\n", signal.len,
          fifo_buffer_len);
  size_t dist = dtw(signal.entries, signal.len, fifo_buffer, fifo_buffer_len);
  LOG_DBG("Distance: %d\n", dist);

  if (dist < SC_CASTER_DIST_THRESHOLD) {
    LOG_DBG("Matched!");
    sc_led_flash(2);
    if (user_callback != NULL) {
      user_callback(0);
    }
  } else {
    LOG_DBG("Not matched!");
    sc_led_flash(3);
  }
  k_msleep(1000);
}

static void sc_caster_thread_fn(void *, void *, void *) {
  struct button_event_queue_el msg;
  struct sc_accel_entry entry;
  while (true) {
    // FIFO is not ready (hopefully).
    if (sc_accel_read(&entry)) {
      continue;
    }
    // LOG_DBG("%10d %10d %10d %10d %10d %10d", entry.ax, entry.ay, entry.az,
    //         entry.gx, entry.gy, entry.gz);

    sc_md_ingest(&md, &entry);

    // We are not capturing.
    if (state == STATE_READY) {
      if (!sc_md_is_still(&md)) {
        LOG_DBG("Will start to capture");
        fifo_buffer_len = 0;
        state = STATE_CAPTURING;
      }
    } else if (state == STATE_CAPTURING) {
      if (fifo_buffer_len == SC_SIGNAL_STORE_MAX_SAMPLES) {
        LOG_DBG("Buffer full. Confirm?");
        state = STATE_CONFIRMING;
        k_msgq_purge(&button_event_msgq);
      } else if (!sc_md_is_still(&md)) {
        fifo_buffer[fifo_buffer_len++] = entry;
      } else {
        LOG_DBG("Stopped capturing. Confirm?");
        state = STATE_CONFIRMING;
        k_msgq_purge(&button_event_msgq);
      }
    } else if (state == STATE_CONFIRMING) {
      // Get button event from queue.
      // if (k_msgq_get(&button_event_msgq, &msg, K_NO_WAIT)) {
      //   // No button event.
      //   continue;
      // }

      // TEST!!
      if (true || (msg.button == SC_BUTTON_A &&
                   msg.event == SC_BUTTON_EVENT_SHORT_PRESS)) {
        LOG_DBG("OK pressed");
        process_buffer();
        fifo_buffer_len = 0;
        state = STATE_WAITING;
      } else if (msg.button == SC_BUTTON_B &&
                 msg.event == SC_BUTTON_EVENT_SHORT_PRESS) {
        LOG_DBG("Cancel pressed");
        fifo_buffer_len = 0;
        state = STATE_WAITING;
      }
    } else if (state == STATE_WAITING) {
      // if (!k_msgq_get(&button_event_msgq, &msg, K_NO_WAIT)) {
      //   // Button event.
      //   if (msg.button == SC_BUTTON_A &&
      //       msg.event == SC_BUTTON_EVENT_SHORT_PRESS) {
      //     LOG_DBG("Set store slot to 0");
      //     sc_led_flash(1);
      //   } else if (msg.button == SC_BUTTON_A &&
      //              msg.event == SC_BUTTON_EVENT_DOUBLE_PRESS) {
      //     LOG_DBG("Set store slot to 1");
      //     sc_led_flash(2);
      //   }
      // }

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