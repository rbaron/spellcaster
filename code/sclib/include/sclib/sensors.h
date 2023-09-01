#ifndef _SC_DATA_H_
#define _SC_DATA_H_

#include "sclib/adc.h"

typedef struct {
  sc_batt_t batt;
} sc_sensors_t;

int sc_sensors_read_all(sc_sensors_t *out);

#endif  // _SC_DATA_H_
