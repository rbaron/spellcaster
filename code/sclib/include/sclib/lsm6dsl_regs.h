#ifndef _SCLIB_LSM6DSL_REGS_H_
#define _SCLIB_LSM6DSL_REGS_H_

#define LSM6DSL_INT_ENABLE
#define LSM6DSL_INT_STATUS
#define LSM6DSL_FIFO_EN
#define LSM6DSL_USER_CTRL
#define LSM6DSL_PWR_MGMT_1
#define LSM6DSL_PWR_MGMT_2
#define LSM6DSL_FIFO_COUNTH
#define LSM6DSL_FIFO_R_W
#define LSM6DSL_INT_PIN_CFG
#define LSM6DSL_WHO_AM_I 0x0f
#define LSM6DSL_SMPLRT_DIV
#define LSM6DSL_GYRO_CONFIG
#define LSM6DSL_CONFIG

#define LSM6DSL_CTRL1_XL 0x10
#define LSM6DSL_CTRL2_G 0x11
#define LSM6DSL_CTRL3_C 0x12
#define LSM6DSL_CTRL4_C 0x13
#define LSM6DSL_CTRL5_C 0x14
#define LSM6DSL_CTRL6_C 0x15
#define LSM6DSL_CTRL7_G 0x16
#define LSM6DSL_CTRL8_XL 0x17
#define LSM6DSL_CTRL9_XL 0x18
#define LSM6DSL_CTRL10_C 0x19

#define LSM5DSL_FIFO_STATUS1 0x3a
#define LSM5DSL_FIFO_STATUS2 0x3b
#define LSM6DSL_FIFO_CTRL3 0x08
#define LSM6DSL_FIFO_CTRL5 0x0a

#define LSM6DSL_MASTER_CONFIG 0x1a
#define LSM6DSL_WAKE_UP_SRC 0x1b
#define LSM6DSL_TAP_SRC 0x1c

#define LSM6DSL_STATUS_REG 0x1e

#define LSM6DSL_FIFO_DATA_OUT_L 0x3e
#define LSM6DSL_FIFO_DATA_OUT_H 0x3f

// Offsets.
#define LSM6DSL_XA_OFFS_H
#define LSM6DSL_YA_OFFS_H
#define LSM6DSL_ZA_OFFS_H
#define LSM6DSL_XG_OFFS_H
#define LSM6DSL_YG_OFFS_H
#define LSM6DSL_ZG_OFFS_H

#endif  // _SCLIB_LSM6DSL_REGS_H_