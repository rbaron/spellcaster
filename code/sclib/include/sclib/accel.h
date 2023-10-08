#ifndef _SCLIB_ACCEL_H_
#define _SCLIB_ACCEL_H_

/*
        Apparently there is already a driver for the MPU6050 in Zephyr:
                                https://github.com/zephyrproject-rtos/zephyr/blob/d11c0a1664f6052e68c6eebd35aa5aba88e7c290/drivers/sensor/mpu6050/mpu6050.c#L146
*/

#include <stdint.h>

#define SC_ACCEL_SAMPLE_RATE_HZ 52

struct sc_accel_entry {
  int16_t ax;
  int16_t ay;
  int16_t az;
  int16_t gx;
  int16_t gy;
  int16_t gz;
};

enum sc_accel_evt {
  SC_ACCEL_WAKEUP_EVT,
  SC_ACCEL_SLEEP_EVT,
};

// Callback for accel events.
typedef void (*sc_accel_evt_handler_t)(enum sc_accel_evt evt);

int sc_accel_init(void);

// Put accel into low power mode.
int sc_accel_sleep(void);

// Sets the callback for accel events.
int sc_accel_set_evt_handler(sc_accel_evt_handler_t handler);

int sc_accel_read(struct sc_accel_entry *entry);

int sc_accel_reset_fifo(void);

#endif  // _SCLIB_ACCEL_H_