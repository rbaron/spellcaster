#include "ble.h"

#include <sclib/macros.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>

#define PRST_MS_TO_INTERVAL(value_ms) ((uint16_t)(value_ms) / 0.625f)

// LOG_MODULE_REGISTER(ble, CONFIG_LOG_DEFAULT_LEVEL);
LOG_MODULE_REGISTER(ble, 4);

static uint8_t service_data[7] = {0};

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_SVC_DATA16, service_data, ARRAY_SIZE(service_data)),
    BT_DATA_BYTES(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME),
};

static bt_addr_le_t mac_addr;

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
  RET_IF_ERR(bt_enable(/*bt_reader_cb_t=*/NULL));
  if (IS_ENABLED(CONFIG_SETTINGS)) {
    RET_IF_ERR_MSG(settings_load(), "Error in settings_load()");
  }
  RET_IF_ERR(get_mac_addr(&mac_addr));
  return 0;
}

int sc_ble_start_advertising() {
  return bt_le_adv_start(
      BT_LE_ADV_PARAM(BT_LE_ADV_OPT_USE_IDENTITY,
                      PRST_MS_TO_INTERVAL(CONFIG_SC_BLE_MIN_ADV_INTERVAL),
                      PRST_MS_TO_INTERVAL(CONFIG_SC_BLE_MAX_ADV_INTERVAL),
                      /*_peer=*/NULL),
      ad, ARRAY_SIZE(ad),
      /*sd=*/NULL,
      /*sd_len=*/0);
}

int sc_ble_stop_advertising() {
  return bt_le_adv_stop();
}

void sc_ble_set_advertising_data(sc_button_t button, sc_button_event_t event) {
  static uint8_t pkt_id = 0;

  // 0xfcd2 - bthome.io service UUID.
  service_data[0] = 0xd2;
  service_data[1] = 0xfc;

  // Service header - no encryption, bt home v2.
  service_data[2] = 0x40;

  // Packet ID.
  service_data[3] = 0x00;
  service_data[4] = pkt_id++;

  // Button.
  service_data[5] = 0x3a;
  // Short press.
  switch (event) {
    case SC_BUTTON_EVENT_SHORT_PRESS:
      service_data[6] = 0x01;
      break;
    case SC_BUTTON_EVENT_DOUBLE_PRESS:
      service_data[6] = 0x02;
      break;
    case SC_BUTTON_EVENT_TRIPLE_PRESS:
      service_data[6] = 0x03;
      break;
    case SC_BUTTON_EVENT_LONG_PRESS:
      service_data[6] = 0x04;
      break;
    default:
      LOG_ERR("Invalid button event: %d", event);
      break;
  }

  LOG_HEXDUMP_DBG(ad, sizeof(ad), "Encoded BLE adv: ");
}