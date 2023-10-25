#include "ble.h"

#include <sclib/macros.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/services/bas.h>
#include <zephyr/bluetooth/services/dis.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>

#include "hid.h"

#define SC_MS_TO_INTERVAL(value_ms) ((uint16_t)(value_ms) / 0.625f)

// LOG_MODULE_REGISTER(ble, CONFIG_LOG_DEFAULT_LEVEL);
LOG_MODULE_REGISTER(ble, LOG_LEVEL_DBG);

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_GAP_APPEARANCE,
                  (CONFIG_BT_DEVICE_APPEARANCE >> 0) & 0xff,
                  (CONFIG_BT_DEVICE_APPEARANCE >> 8) & 0xff),
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(BT_UUID_HIDS_VAL),
                  BT_UUID_16_ENCODE(BT_UUID_BAS_VAL)),
};

static const struct bt_data sd[] = {
    BT_DATA_BYTES(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME),
};

static bt_addr_le_t mac_addr;

static struct k_work pairing_work;
struct pairing_data_mitm {
  struct bt_conn *conn;
  unsigned int passkey;
};

K_MSGQ_DEFINE(mitm_queue, sizeof(struct pairing_data_mitm),
              CONFIG_BT_HIDS_MAX_CLIENT_COUNT, /*max_msgs=*/4);

static struct conn_mode {
  struct bt_conn *conn;
  bool in_boot_mode;
} conn_mode[CONFIG_BT_HIDS_MAX_CLIENT_COUNT];

/*
 * Connection callbacks.
 */
static void connected(struct bt_conn *conn, uint8_t err) {
  char addr[BT_ADDR_LE_STR_LEN];

  bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

  if (err) {
    LOG_ERR("Failed to connect to %s (%u)\n", addr, err);
    return;
  }

  LOG_INF("Connected %s\n", addr);

  if (bt_hids_connected(sc_hid_get_hids_obj(), conn)) {
    LOG_ERR("Failed to notify HID service about connection\n");
    return;
  }

  if (err) {
    LOG_ERR("Failed to notify HID service about connection\n");
    return;
  }

  for (size_t i = 0; i < CONFIG_BT_HIDS_MAX_CLIENT_COUNT; i++) {
    if (!conn_mode[i].conn) {
      conn_mode[i].conn = conn;
      conn_mode[i].in_boot_mode = false;
      break;
    }
  }
}

static void disconnected(struct bt_conn *conn, uint8_t reason) {
  int err;
  bool is_any_dev_connected = false;
  char addr[BT_ADDR_LE_STR_LEN];

  bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

  // 19 == BT_HCI_ERR_REMOTE_USER_TERM_CONN == 0x13
  LOG_INF("Disconnected from %s (reason %u)\n", addr, reason);

  err = bt_hids_disconnected(sc_hid_get_hids_obj(), conn);

  if (err) {
    LOG_ERR("Failed to notify HID service about disconnection\n");
  }

  for (size_t i = 0; i < CONFIG_BT_HIDS_MAX_CLIENT_COUNT; i++) {
    if (conn_mode[i].conn == conn) {
      conn_mode[i].conn = NULL;
    } else {
      if (conn_mode[i].conn) {
        is_any_dev_connected = true;
      }
    }
  }

  sc_ble_start_advertising();
}

static void security_changed(struct bt_conn *conn, bt_security_t level,
                             enum bt_security_err err) {
  char addr[BT_ADDR_LE_STR_LEN];
  bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
  if (!err) {
    LOG_INF("Security changed: %s level %u\n", addr, level);
  } else {
    LOG_ERR("Security failed: %s level %u err %d\n", addr, level, err);
  }
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
    .security_changed = security_changed,
};

/*
 * Pairing callbacks.
 */

static void pairing_process(struct k_work *work) {
  int err;
  struct pairing_data_mitm pairing_data;

  char addr[BT_ADDR_LE_STR_LEN];

  err = k_msgq_peek(&mitm_queue, &pairing_data);
  if (err) {
    return;
  }

  bt_addr_le_to_str(bt_conn_get_dst(pairing_data.conn), addr, sizeof(addr));

  LOG_WRN("Passkey for %s: %06u\n", addr, pairing_data.passkey);
  LOG_WRN("Press Button 1 to confirm, Button 2 to reject.\n");
}

static void pairing_complete(struct bt_conn *conn, bool bonded) {
  char addr[BT_ADDR_LE_STR_LEN];

  bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

  LOG_WRN("Pairing completed: %s, bonded: %d\n", addr, bonded);
}

static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason) {
  char addr[BT_ADDR_LE_STR_LEN];
  struct pairing_data_mitm pairing_data;

  if (k_msgq_peek(&mitm_queue, &pairing_data) != 0) {
    return;
  }

  if (pairing_data.conn == conn) {
    bt_conn_unref(pairing_data.conn);
    k_msgq_get(&mitm_queue, &pairing_data, K_NO_WAIT);
  }

  bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

  LOG_WRN("Pairing failed conn: %s, reason %d\n", addr, reason);
}

static struct bt_conn_auth_info_cb conn_auth_info_callbacks = {
    .pairing_complete = pairing_complete, .pairing_failed = pairing_failed};

// bt_addr_le_t.a holds the MAC address in big-endian.
static int get_mac_addr(bt_addr_le_t *out) {
  struct bt_le_oob oob;
  RET_IF_ERR(bt_le_oob_get_local(BT_ID_DEFAULT, &oob));
  const uint8_t *addr = oob.addr.a.val;
  LOG_INF("MAC Address: %02x:%02x:%02x:%02x:%02x:%02x", addr[5], addr[4],
          addr[3], addr[2], addr[1], addr[0]);
  *out = oob.addr;
  return 0;
}

int sc_ble_init() {
  RET_IF_ERR(bt_conn_auth_info_cb_register(&conn_auth_info_callbacks));
  RET_IF_ERR(bt_enable(/*bt_reader_cb_t=*/NULL));
  if (IS_ENABLED(CONFIG_SETTINGS)) {
    int err = settings_load();
    if (err) {
      LOG_ERR("Cannot load settings from flash (err %d)", err);
    }
  }
  RET_IF_ERR(get_mac_addr(&mac_addr));
  k_work_init(&pairing_work, pairing_process);
  return 0;
}

int sc_ble_start_advertising() {
  return bt_le_adv_start(
      BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_ONE_TIME,
                      SC_MS_TO_INTERVAL(CONFIG_SC_BLE_MIN_ADV_INTERVAL),
                      SC_MS_TO_INTERVAL(CONFIG_SC_BLE_MAX_ADV_INTERVAL),
                      /*_peer=*/NULL),
      ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
}

int sc_ble_stop_advertising() {
  return bt_le_adv_stop();
}

int sc_ble_send_button_press(struct bt_hids *hids_obj, uint8_t button) {
  uint8_t data[INPUT_REPORT_KEYS_MAX_LEN] = {0};
  data[0] = 0x00;
  data[1] = 0x00;
  data[2] = button;
  for (size_t i = 0; i < CONFIG_BT_HIDS_MAX_CLIENT_COUNT; i++) {
    if (conn_mode[i].conn) {
      LOG_DBG("Sending button to conn %d", i);
      RET_IF_ERR(bt_hids_inp_rep_send(hids_obj, conn_mode[i].conn,
                                      INPUT_REP_KEYS_IDX, data, sizeof(data),
                                      NULL));
    }
  }
  return 0;
}

int sc_ble_send_button_release(struct bt_hids *hids_obj, uint8_t button) {
  uint8_t data[INPUT_REPORT_KEYS_MAX_LEN] = {0};
  for (size_t i = 0; i < CONFIG_BT_HIDS_MAX_CLIENT_COUNT; i++) {
    if (conn_mode[i].conn) {
      LOG_DBG("Sending button to conn %d", i);
      RET_IF_ERR(bt_hids_inp_rep_send(hids_obj, conn_mode[i].conn,
                                      INPUT_REP_KEYS_IDX, data, sizeof(data),
                                      NULL));
    }
  }
  return 0;
}