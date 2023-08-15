#ifndef _SC_BLE_H_
#define _SC_BLE_H_

#include <stdint.h>

int sc_ble_init(void);

int sc_ble_start_advertising(void);

int sc_ble_stop_advertising(void);

int sc_ble_set_advertising_data(uint8_t n_presses);

#endif  // _SC_BLE_H_