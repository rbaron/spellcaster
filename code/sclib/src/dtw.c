#include "sclib/dtw.h"

#include <math.h>
#include <string.h>

#include "sclib/accel.h"
#include "sclib/motion_detector.h"

// 3s of data at 52 Hz minus 500ms removed from stillness detection.
#define IMU_ENTRIES_MAX \
  (SC_ACCEL_SAMPLE_RATE_HZ * (3000 - SC_MD_HORIZ_TIMER_PERIOD_MS) / 1000)

#define IDX(alen, blen, r, c) ((r) * (blen) + (c))

#define MIN(x, y) (((x) < (y)) ? (x) : (y))

static inline size_t dist(const struct sc_accel_entry *a,
                          const struct sc_accel_entry *b) {
  return sqrt(pow(a->ax - b->ax, 2) + pow(a->ay - b->ay, 2) +
              pow(a->az - b->az, 2) + pow(a->gx - b->gx, 2) +
              pow(a->gy - b->gy, 2) + pow(a->gz - b->gz, 2));
}

size_t dtw(const struct sc_accel_entry *a, size_t a_len,
           const struct sc_accel_entry *b, size_t b_len) {
  // This eats up RAM FAST.
  // TODO: consider FastDTW: https://cs.fit.edu/~pkc/papers/tdm04.pdf,
  // which runs in O(n) space and time (!!).
  static size_t DTW[IMU_ENTRIES_MAX * IMU_ENTRIES_MAX];

  // Ideally set to infinity, but that's too large.
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