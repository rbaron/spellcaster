#include "sclib/dtw.h"

#include <math.h>
#include <string.h>

#include "sclib/accel.h"
#include "sclib/signal_store.h"

#if CONFIG_SCLIB_DTW_USE_WINDOWING
#define DTW_WINDOW_SIZE CONFIG_SCLIB_DTW_WINDOW_SIZE
#define DTW_WIDTH (2 * DTW_WINDOW_SIZE + 1)
#define DTW_SIZE (SC_SIGNAL_STORE_MAX_SAMPLES * DTW_WIDTH)
#define IDX(alen, blen, r, c) ((r) * (DTW_WIDTH) + (c))
#else  // No windowing.
#define DTW_SIZE (SC_SIGNAL_STORE_MAX_SAMPLES * SC_SIGNAL_STORE_MAX_SAMPLES)
#define IDX(alen, blen, r, c) ((r) * (blen) + (c))
#endif

static inline size_t dist(const struct sc_accel_entry *a,
                          const struct sc_accel_entry *b) {
  return sqrt(pow(a->ax - b->ax, 2) + pow(a->ay - b->ay, 2) +
              pow(a->az - b->az, 2) + pow(a->gx - b->gx, 2) +
              pow(a->gy - b->gy, 2) + pow(a->gz - b->gz, 2));
}

size_t dtw(const struct sc_accel_entry *a, size_t a_len,
           const struct sc_accel_entry *b, size_t b_len) {
  // This bad boy eats up RAM fast.
  static size_t DTW[DTW_SIZE];

  // Ideally set to infinity, but that's too large.
  memset(DTW, 0xff, sizeof(DTW));

  DTW[IDX(a_len, b_len, 0, 0)] = 0;

  for (size_t i = 0; i < a_len; ++i) {
#if CONFIG_SCLIB_DTW_USE_WINDOWING
    for (size_t j = MAX(0, i - DTW_WINDOW_SIZE);
         j < MIN(b_len, i + DTW_WINDOW_SIZE); ++j) {
#else
    for (size_t j = 0; j < b_len; ++j) {
#endif
      size_t cost = dist(&a[i], &b[j]);
      DTW[IDX(a_len, b_len, i, j)] =
          cost + MIN(DTW[IDX(a_len, b_len, i - 1, j)],
                     MIN(DTW[IDX(a_len, b_len, i, j - 1)],
                         DTW[IDX(a_len, b_len, i - 1, j - 1)]));
    }
  }

  // Normalized path cost. Dont' change this regardless of windowing, since it
  // would mess with the threshold calibration.
  return DTW[IDX(a_len, b_len, a_len - 1, b_len - 1)] / (a_len + b_len);
}