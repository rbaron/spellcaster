#ifndef _SC_SHT3C_H_
#define _SC_SHT3C_H_

// Values from the SHTC3 datasheet.
#define SC_SHTC3_ADDR 0x70
#define SC_SHTC3_CMD_SLEEP 0xb098
#define SC_SHTC3_CMD_WAKEUP 0x3517
#define SC_SHTC3_CMD_MEASURE_TFIRST_LOW_POWER 0x609c
#define SC_SHTC3_CMD_MEASURE_TFIRST_NORMAL 0x7866

typedef struct {
  // Temperature in Celcius.
  float temp_c;
  // Relative humidity in [0, 1.0].
  float rel_humi;
} sc_shtc3_read_t;

int sc_shtc3_read(sc_shtc3_read_t *out);

#endif  // _SC_SHT3C_H_