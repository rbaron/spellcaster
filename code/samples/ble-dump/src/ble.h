#ifndef _SC_BLE_H_
#define _SC_BLE_H_

#include <stdint.h>

int sc_ble_init(void);

int sc_ble_send(uint8_t *data, uint16_t len);

#endif  // _SC_BLE_H_