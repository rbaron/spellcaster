#ifndef _SC_DATA_H_
#define _SC_DATA_H_

#include "sclib/adc.h"
#include "sclib/shtc3.h"

typedef struct {
  sc_batt_t batt;
  sc_shtc3_read_t shtc3;
} sc_sensors_t;

int sc_sensors_read_all(sc_sensors_t *out);

#endif  // _SC_DATA_H_
