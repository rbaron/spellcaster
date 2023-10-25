#ifndef _SC_SIGNAL_STORE_H_
#define _SC_SIGNAL_STORE_H_

#include <stdint.h>
#include <string.h>

#include "sclib/accel.h"
#include "sclib/motion_detector.h"

// Mega dirty hack for Zigbee. Not enough RAM.
#if CONFIG_ZIGBEE
#define SC_SIGNAL_STORE_MAX_SAMPLES (SC_ACCEL_SAMPLE_RATE_HZ * (2100) / 1000)
#else
// Store ~2.4 seconds of data.
// #define SC_SIGNAL_STORE_MAX_SAMPLES (SC_ACCEL_SAMPLE_RATE_HZ * (2400) / 1000)
#define SC_SIGNAL_STORE_MAX_SAMPLES (SC_ACCEL_SAMPLE_RATE_HZ * (2400) / 1000)
#endif

// Number of slots for storing signals.
#define SC_SIGNAL_STORE_MAX_SIGNALS 5

struct sc_signal {
  struct sc_accel_entry entries[SC_SIGNAL_STORE_MAX_SAMPLES];
  size_t len;
};

int sc_ss_init(void);

int sc_ss_store(uint8_t slot, const struct sc_signal *signal);

int sc_ss_load(uint8_t slot, struct sc_signal *signal);

#endif  // _SC_SIGNAL_STORE_H_