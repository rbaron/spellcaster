#ifndef _SC_SIGNAL_STORE_H_
#define _SC_SIGNAL_STORE_H_

#include <stdint.h>
#include <string.h>

#include "sclib/accel.h"

// Store 3 seconds of data.
#define SC_SIGNAL_STORE_MAX_SAMPLES (3 * SC_ACCEL_SAMPLE_RATE_HZ)

struct sc_signal {
  struct sc_accel_entry entries[SC_SIGNAL_STORE_MAX_SAMPLES];
  size_t len;
};

int sc_ss_init(void);

int sc_ss_store(uint8_t slot, const struct sc_signal *signal);

int sc_ss_load(uint8_t slot, struct sc_signal *signal);

#endif  // _SC_SIGNAL_STORE_H_