#ifndef _SC_BLE_H_
#define _SC_BLE_H_

#include <bluetooth/services/hids.h>

int sc_ble_init();

int sc_ble_start_advertising();

int sc_ble_stop_advertising();

int sc_ble_send_button_press(struct bt_hids *hids_obj, uint8_t button);

int sc_ble_send_button_release(struct bt_hids *hids_obj, uint8_t button);

#endif  // _SC_BLE_H_