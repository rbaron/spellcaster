#include "sclib/button.h"

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "sclib/macros.h"

#define SC_DEBOUNCING_INTERVAL K_MSEC(5)
#define SC_LONG_PRESS_DURATION K_MSEC(250)
#define SC_DOUBLE_PRESS_CB_INTERVAL K_MSEC(400)

LOG_MODULE_REGISTER(button, CONFIG_SCLIB_LOG_LEVEL);

typedef struct {
  sc_button_t button;
  bool handled_long_press;
  uint8_t n_presses;
  struct gpio_dt_spec button_dt;
  struct gpio_callback cb_data;
  // Used for debouncing.
  struct k_work_delayable debouncing_delayable;
  struct k_work_delayable double_press_delayable;
  struct k_work_delayable long_press_delayable;
} button_cfg_t;

static button_cfg_t button_cfgs[] = {
    {.button = SC_BUTTON_A,
     .handled_long_press = false,
     .n_presses = 0,
     .button_dt = GPIO_DT_SPEC_GET(DT_NODELABEL(button_a), gpios)},
};

static sc_button_callback_t user_callback = NULL;

static void maybe_call_user_callback(sc_button_t button,
                                     sc_button_event_t event) {
  LOG_DBG("Calling user callback for button %d, event %d", (int)button,
          (int)event);
  if (user_callback != NULL) {
    user_callback(button, event);
  } else {
    LOG_WRN("No user callback registered for button %d", button);
  }
}
static void double_press_cb(struct k_work *work) {
  struct k_work_delayable *delayable =
      CONTAINER_OF(work, struct k_work_delayable, work);
  button_cfg_t *cfg =
      CONTAINER_OF(delayable, button_cfg_t, double_press_delayable);
  uint8_t n_presses = cfg->n_presses;
  cfg->n_presses = 0;

  if (n_presses == 1) {
    LOG_DBG("Button %d single press event", cfg->button);
    maybe_call_user_callback(cfg->button, SC_BUTTON_EVENT_SHORT_PRESS);
  } else if (n_presses == 2) {
    LOG_DBG("Button %d double press event", cfg->button);
    maybe_call_user_callback(cfg->button, SC_BUTTON_EVENT_DOUBLE_PRESS);
  } else if (n_presses == 3) {
    LOG_DBG("Button %d triple press event", cfg->button);
    maybe_call_user_callback(cfg->button, SC_BUTTON_EVENT_TRIPLE_PRESS);
  } else if (n_presses == 4) {
    LOG_DBG("Button %d quadruple press event", cfg->button);
    maybe_call_user_callback(cfg->button, SC_BUTTON_EVENT_QUADRUPLE_PRESS);
  } else if (n_presses == 5) {
    LOG_DBG("Button %d quintuple press event", cfg->button);
    maybe_call_user_callback(cfg->button, SC_BUTTON_EVENT_QUINTUPLE_PRESS);
  } else if (n_presses == 6) {
    LOG_DBG("Button %d sextuple press event", cfg->button);
    maybe_call_user_callback(cfg->button, SC_BUTTON_EVENT_SEXTUPLE_PRESS);
  } else {
    LOG_WRN("Button %d unexpected number of presses: %d", cfg->button,
            n_presses);
  }
}

static void long_press_cb(struct k_work *work) {
  struct k_work_delayable *delayable =
      CONTAINER_OF(work, struct k_work_delayable, work);
  button_cfg_t *cfg =
      CONTAINER_OF(delayable, button_cfg_t, long_press_delayable);
  cfg->handled_long_press = true;
  maybe_call_user_callback(cfg->button, SC_BUTTON_EVENT_LONG_PRESS);
}

static void button_pressed_cb(struct k_work *work) {
  struct k_work_delayable *delayable =
      CONTAINER_OF(work, struct k_work_delayable, work);
  button_cfg_t *cfg =
      CONTAINER_OF(delayable, button_cfg_t, debouncing_delayable);
  int button_active = sc_button_poll(cfg->button);
  LOG_DBG("Button %d pressed, handled_long_press %d, n_presses %d, active %d",
          cfg->button, cfg->handled_long_press, cfg->n_presses, button_active);

  if (button_active) {
    // First press, schedule long press callback.
    if (cfg->n_presses == 0) {
      LOG_DBG("Scheduling long press callback");
      k_work_reschedule(&cfg->long_press_delayable, SC_LONG_PRESS_DURATION);
    }
    // Button inactive.
  } else {
    // Sync?
    // Cancel if work has not ran yet.
    LOG_DBG("Canceling long press callback");
    k_work_cancel_delayable(&cfg->long_press_delayable);
    // We only consider short button presses if it didn't run the long press
    // handler.
    if (!cfg->handled_long_press) {
      LOG_DBG("Button %d scheduling double_press_delayable", cfg->button);
      cfg->n_presses += 1;
      k_work_reschedule(&cfg->double_press_delayable,
                        SC_DOUBLE_PRESS_CB_INTERVAL);
    } else {
      LOG_DBG("Button %d handled long press, ignoring short press",
              cfg->button);
      cfg->handled_long_press = false;
    }
  }
}

static void button_pressed_isr(const struct device *dev,
                               struct gpio_callback *cb, uint32_t pins) {
  button_cfg_t *cfg = CONTAINER_OF(cb, button_cfg_t, cb_data);
  k_work_reschedule(&cfg->debouncing_delayable, SC_DEBOUNCING_INTERVAL);
}

int sc_button_init() {
  for (int i = 0; i < ARRAY_SIZE(button_cfgs); i++) {
    button_cfg_t *cfg = &button_cfgs[i];
    RET_IF_ERR(!device_is_ready(cfg->button_dt.port));
    RET_IF_ERR(gpio_pin_configure_dt(&cfg->button_dt, GPIO_INPUT));
    k_work_init_delayable(&cfg->debouncing_delayable, button_pressed_cb);
    k_work_init_delayable(&cfg->double_press_delayable, double_press_cb);
    k_work_init_delayable(&cfg->long_press_delayable, long_press_cb);
    // EDGE interrupts seem to consume more power than LEVEL ones.
    // For GPIO_INT_EDGE_BOTH: 16 uA idle.
    // For GPIO_INT_LEVEL_ACTIVE: 3 uA idle.
    // Related issue:
    // https://github.com/zephyrproject-rtos/zephyr/issues/28499
    // Apparently sense-edge-mask brings the power consumption down to
    // 3 uA for EDGE interrupts too.
    RET_IF_ERR(
        gpio_pin_interrupt_configure_dt(&cfg->button_dt, GPIO_INT_EDGE_BOTH));
    gpio_init_callback(&cfg->cb_data, button_pressed_isr,
                       BIT(cfg->button_dt.pin));
    RET_IF_ERR(gpio_add_callback(cfg->button_dt.port, &cfg->cb_data));
  }
  return 0;
}

int sc_button_register_callback(sc_button_callback_t callback) {
  user_callback = callback;
  return 0;
}

int sc_button_poll(sc_button_t sc_button) {
  button_cfg_t *cfg = &button_cfgs[sc_button];
  return gpio_pin_get_dt(&cfg->button_dt);
}
