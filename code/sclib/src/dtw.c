#include "sclib/dtw.h"

#include <math.h>
#include <string.h>

#define IMU_ENTRIES_MAX 150

#define IDX(alen, blen, r, c) ((r) * (blen) + (c))

#define MIN(x, y) (((x) < (y)) ? (x) : (y))

// // TODO: remove this hack.
// static size_t pow(size_t x, size_t y) {
//   // size_t result = 1;
//   // for (size_t i = 0; i < y; ++i) {
//   //   result *= x;
//   // }
//   // return result;
//   return x * x;
// }

static inline size_t dist(const struct sc_accel_entry *a,
                          const struct sc_accel_entry *b) {
  return sqrt(pow(a->ax - b->ax, 2) + pow(a->ay - b->ay, 2) +
              pow(a->az - b->az, 2) + pow(a->gx - b->gx, 2) +
              pow(a->gy - b->gy, 2) + pow(a->gz - b->gz, 2));
}

size_t dtw(const struct sc_accel_entry *a, size_t a_len,
           const struct sc_accel_entry *b, size_t b_len) {
  static size_t DTW[IMU_ENTRIES_MAX * IMU_ENTRIES_MAX];

  // WHAT?
  // memset(DTW, UINT32_MAX, sizeof(DTW));
  memset(DTW, 0xff, sizeof(DTW));

  DTW[IDX(a_len, b_len, 0, 0)] = 0;

  for (size_t i = 0; i < a_len; ++i) {
    for (size_t j = 0; j < b_len; ++j) {
      size_t cost = dist(&a[i], &b[j]);

      DTW[IDX(a_len, b_len, i, j)] =
          cost + MIN(DTW[IDX(a_len, b_len, i - 1, j)],
                     MIN(DTW[IDX(a_len, b_len, i, j - 1)],
                         DTW[IDX(a_len, b_len, i - 1, j - 1)]));
    }
  }

  // Normalized path cost.
  return DTW[IDX(a_len, b_len, a_len - 1, b_len - 1)] / (a_len + b_len);
}