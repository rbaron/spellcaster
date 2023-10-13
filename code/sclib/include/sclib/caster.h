#ifndef _SCLIB_CASTER_H_
#define _SCLIB_CASTER_H_

#include <stddef.h>
#include <stdint.h>

#include "sclib/accel.h"

typedef void (*sc_caster_callback_t)(uint8_t slot);

int sc_caster_init(sc_caster_callback_t callback);

typedef void (*sc_caster_signal_callback_t)(float initial_row_angle,
                                            const struct sc_accel_entry *entry,
                                            size_t len);

int sc_caster_set_signal_callback(sc_caster_signal_callback_t callback);

#endif  // _SCLIB_CASTER_H_