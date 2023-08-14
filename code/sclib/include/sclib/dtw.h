#ifndef _DTW_H_
#define _DTW_H_

#include <stddef.h>
#include <stdint.h>

#include "sclib/accel.h"

size_t dtw(const struct sc_accel_entry *a, size_t a_len,
           const struct sc_accel_entry *b, size_t b_len);

#endif  // _DTW_H_