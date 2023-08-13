#ifndef _SC_ADC_H_
#define _SC_ADC_H_

#include <stdint.h>

typedef struct {
  int16_t raw;
  int32_t millivolts;
  float voltage;
} sc_adc_read_t;

typedef struct {
  sc_adc_read_t adc_read;
  float percentage;
} sc_batt_t;

int sc_adc_init();

int sc_adc_batt_read(sc_batt_t* out);

#endif  // _SC_ADC_H_