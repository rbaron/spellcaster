#include <zephyr/drivers/gpio.h>

#include "sclib/led.h"
#include "sclib/macros.h"

LOG_MODULE_REGISTER(vib, CONFIG_SCLIB_LOG_LEVEL);

struct gpio_dt_spec vib = GPIO_DT_SPEC_GET(DT_NODELABEL(vib0), gpios);

int sc_vib_init() {
  RET_IF_ERR(!device_is_ready(vib.port));
  return gpio_pin_configure_dt(&vib, GPIO_OUTPUT);
}