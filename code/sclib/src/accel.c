#include "sclib/accel.h"

#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "sclib/macros.h"
#include "sclib/mpu6050_regs.h"

#define PARSES16(buf, i) (((int16_t)buf[i] << 8) | (int16_t)buf[i + 1])
#define PARSEU16(buf, i) (((uint16_t)buf[i] << 8) | (uint16_t)buf[i + 1])

LOG_MODULE_REGISTER(accel, CONFIG_SCLIB_LOG_LEVEL);

static const struct i2c_dt_spec mpu = I2C_DT_SPEC_GET(DT_NODELABEL(mpu));

static const int16_t calibration_offsets[6] = {-1708, 755, 1014, 114, 32, -3};

static int write_reg_16(uint8_t reg, int16_t val) {
  uint8_t buf[3];
  buf[0] = reg;
  buf[1] = val >> 8;
  buf[2] = val & 0xff;
  return i2c_write_dt(&mpu, buf, sizeof(buf));
}

int sc_accel_init(void) {
  RET_IF_ERR_MSG(!device_is_ready(mpu.bus), "MPU6050 is not ready");

  // Reset.
  RET_IF_ERR(i2c_reg_write_byte_dt(&mpu, MPU6050_PWR_MGMT_1, 0x80));

  // Upload offsets. Not sure this is actually helpful.
  RET_IF_ERR(write_reg_16(MPU6050_XA_OFFS_H, calibration_offsets[0]));
  RET_IF_ERR(write_reg_16(MPU6050_YA_OFFS_H, calibration_offsets[1]));
  RET_IF_ERR(write_reg_16(MPU6050_ZA_OFFS_H, calibration_offsets[2]));
  RET_IF_ERR(write_reg_16(MPU6050_XG_OFFS_H, calibration_offsets[3]));
  RET_IF_ERR(write_reg_16(MPU6050_YG_OFFS_H, calibration_offsets[4]));
  RET_IF_ERR(write_reg_16(MPU6050_ZG_OFFS_H, calibration_offsets[5]));

  // Set clock source x gyro.
  RET_IF_ERR(i2c_reg_write_byte_dt(&mpu, MPU6050_PWR_MGMT_1, 0x01));
  // Set gyro range to +/- 500 deg/s.
  RET_IF_ERR(i2c_reg_write_byte_dt(&mpu, MPU6050_GYRO_CONFIG, 0x01));
  // Enable accel and gyro in fifo.
  RET_IF_ERR(i2c_reg_write_byte_dt(&mpu, MPU6050_FIFO_EN, 0x78));
  // Set fifo rate to 50 Hz.
  RET_IF_ERR(i2c_reg_write_byte_dt(&mpu, MPU6050_SMPLRT_DIV, 159));
  // Enable and reset fifo.
  RET_IF_ERR(i2c_reg_write_byte_dt(&mpu, MPU6050_USER_CTRL, 0x44));
  return 0;
}

int sc_accel_read(struct sc_accel_entry *entry) {
  // Read fifo count.
  uint8_t write_buf[1] = {MPU6050_FIFO_COUNTH};
  uint8_t read_buf[2];
  RET_IF_ERR(i2c_write_read_dt(&mpu, write_buf, 1, read_buf, 2));
  uint16_t fifo_count = PARSEU16(read_buf, 0);

  // Read fifo.
  if (fifo_count <= 0) {
    return -1;
  }

  uint8_t fifo_buf[sizeof(int16_t) * 6];
  write_buf[0] = MPU6050_FIFO_R_W;
  RET_IF_ERR(i2c_write_read_dt(&mpu, write_buf, 1, fifo_buf, sizeof(fifo_buf)));

  entry->ax = PARSES16(fifo_buf, 0);
  entry->ay = PARSES16(fifo_buf, 2);
  entry->az = PARSES16(fifo_buf, 4);
  entry->gx = PARSES16(fifo_buf, 6);
  entry->gy = PARSES16(fifo_buf, 8);
  entry->gz = PARSES16(fifo_buf, 10);

  return 0;
}
