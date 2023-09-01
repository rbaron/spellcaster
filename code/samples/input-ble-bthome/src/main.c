#include <sclib/button.h>
#include <sclib/led.h>
#include <sclib/macros.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#include "ble.h"

LOG_MODULE_REGISTER(app, CONFIG_LOG_DEFAULT_LEVEL);

#define SC_BUTTON_PRESS_QUEUE_SIZE 8

typedef struct {
  sc_button_t button;
  sc_button_event_t event;
} button_press_queue_element_t;

K_MSGQ_DEFINE(button_press_queue, sizeof(button_press_queue_element_t),
              SC_BUTTON_PRESS_QUEUE_SIZE, 4);

static void button_press_cb(sc_button_t button, sc_button_event_t event) {
  LOG_DBG("button_press_cb: button %d - event %d", button, event);
  button_press_queue_element_t qel = {.button = button, .event = event};
  k_msgq_put(&button_press_queue, &qel, K_FOREVER);
}

static void broadcast_button_press(sc_button_t button,
                                   sc_button_event_t event) {
  sc_led_flash(/*times=*/1);
  sc_ble_set_advertising_data(button, event);
  sc_ble_start_advertising();
  k_msleep(CONFIG_SC_BLE_ADV_DURATION_MSEC);
  sc_ble_stop_advertising();
}

void main(void) {
  __ASSERT_NO_MSG(!sc_button_init());
  __ASSERT_NO_MSG(!sc_led_init());
  __ASSERT_NO_MSG(!sc_ble_init());

  __ASSERT_NO_MSG(!sc_button_register_callback(button_press_cb));

  sc_led_flash(/*times=*/2);

  button_press_queue_element_t qel;
  while (1) {
    k_msgq_get(&button_press_queue, &qel, K_FOREVER);
    LOG_DBG("main: button %d - event %d", qel.button, qel.event);
    broadcast_button_press(qel.button, qel.event);
  }
}
