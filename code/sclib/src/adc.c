#include "sclib/adc.h"

#include <math.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "sclib/macros.h"

LOG_MODULE_REGISTER(adc, CONFIG_SCLIB_LOG_LEVEL);

// Shared buffer and adc_sequennce.
static int16_t buf;
static struct adc_sequence sequence = {
    .buffer = &buf,
    .buffer_size = sizeof(buf),
};

static const struct adc_dt_spec adc_batt_spec =
    ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 1);

typedef struct {
  // High (h) and low (p) voltage (v) and % (p) points.
  float vh, vl, ph, pl;
} batt_disch_linear_section_t;

static void set_battery_percent(const sc_adc_read_t* read, sc_batt_t* out) {
  // Must be sorted by .vh.
  static const batt_disch_linear_section_t sections[] = {
      {.vh = 3.00f, .vl = 2.90f, .ph = 1.00f, .pl = 0.42f},
      {.vh = 2.90f, .vl = 2.74f, .ph = 0.42f, .pl = 0.18f},
      {.vh = 2.74f, .vl = 2.44f, .ph = 0.18f, .pl = 0.06f},
      {.vh = 2.44f, .vl = 2.01f, .ph = 0.06f, .pl = 0.00f},
  };

  const float v = read->voltage;

  if (v > sections[0].vh) {
    out->percentage = 1.0f;
    return;
  }
  for (int i = 0; i < ARRAY_SIZE(sections); i++) {
    const batt_disch_linear_section_t* s = &sections[i];
    if (v > s->vl) {
      out->percentage =
          s->pl + (v - s->vl) * ((s->ph - s->pl) / (s->vh - s->vl));
      return;
    }
  }
  out->percentage = 0.0f;
  return;
}

static int read_adc_spec(const struct adc_dt_spec* spec, sc_adc_read_t* out) {
  RET_IF_ERR(adc_sequence_init_dt(spec, &sequence));

  RET_IF_ERR(adc_read(spec->dev, &sequence));

  int32_t val_mv = buf;
  RET_IF_ERR(adc_raw_to_millivolts_dt(spec, &val_mv));

  out->raw = buf;
  out->millivolts = val_mv;
  out->voltage = val_mv / 1000.0f;
  return 0;
}

int sc_adc_init() {
  RET_IF_ERR(adc_channel_setup_dt(&adc_batt_spec));
  return 0;
}

int sc_adc_batt_read(sc_batt_t* out) {
  RET_IF_ERR(read_adc_spec(&adc_batt_spec, &out->adc_read));
  set_battery_percent(&out->adc_read, out);
  return 0;
}
