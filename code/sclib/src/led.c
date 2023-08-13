#include "sclib/led.h"

#include <zephyr/drivers/gpio.h>

#include "sclib/macros.h"

LOG_MODULE_REGISTER(led, CONFIG_SCLIB_LOG_LEVEL);

struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_NODELABEL(led0), gpios);

int sc_led_init() {
  RET_IF_ERR(!device_is_ready(led.port));
  return gpio_pin_configure_dt(&led, GPIO_OUTPUT);
}