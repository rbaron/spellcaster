#include "sclib/sensors.h"

#include <zephyr/logging/log.h>

#include "sclib/adc.h"
#include "sclib/led.h"
#include "sclib/macros.h"

LOG_MODULE_REGISTER(sensors, CONFIG_SCLIB_LOG_LEVEL);

int sc_sensors_read_all(sc_sensors_t *sensors) {
  RET_IF_ERR(sc_adc_batt_read(&sensors->batt));

  LOG_DBG("Batt: %d mV (%.2f%%)", sensors->batt.adc_read.millivolts,
          100 * sensors->batt.percentage);
  LOG_DBG("--------------------------------------------------");

  return 0;
}