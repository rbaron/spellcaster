#ifndef _SC_BLE_H_
#define _SC_BLE_H_

#include <sclib/button.h>

int sc_ble_init();

int sc_ble_start_advertising();

int sc_ble_stop_advertising();

void sc_ble_set_advertising_data(sc_button_t button, sc_button_event_t event);

#endif  // _SC_BLE_H_