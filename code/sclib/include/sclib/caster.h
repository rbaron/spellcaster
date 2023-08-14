#ifndef _SCLIB_CASTER_H_
#define _SCLIB_CASTER_H_

#include <stdint.h>

typedef void (*sc_caster_callback_t)(uint8_t slot);

int sc_caster_init(sc_caster_callback_t callback);

#endif  // _SCLIB_CASTER_H_