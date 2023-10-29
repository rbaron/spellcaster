#include "ble.h"

#include <sclib/macros.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>

#define PRST_MS_TO_INTERVAL(value_ms) ((uint16_t)(value_ms) / 0.625f)

// LOG_MODULE_REGISTER(ble, CONFIG_LOG_DEFAULT_LEVEL);
LOG_MODULE_REGISTER(ble, 4);

static uint8_t service_data[8] = {0};

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

int sc_ble_init(void) {
  RET_IF_ERR(bt_enable(/*bt_reader_cb_t=*/NULL));
  if (IS_ENABLED(CONFIG_SETTINGS)) {
    RET_IF_ERR_MSG(settings_load(), "Error in settings_load()");
  }
  RET_IF_ERR(get_mac_addr(&mac_addr));
  return 0;
}

int sc_ble_start_advertising(void) {
  return bt_le_adv_start(
      BT_LE_ADV_PARAM(BT_LE_ADV_OPT_USE_IDENTITY,
                      PRST_MS_TO_INTERVAL(CONFIG_SC_BLE_MIN_ADV_INTERVAL),
                      PRST_MS_TO_INTERVAL(CONFIG_SC_BLE_MAX_ADV_INTERVAL),
                      /*_peer=*/NULL),
      ad, ARRAY_SIZE(ad),
      /*sd=*/NULL,
      /*sd_len=*/0);
}

int sc_ble_stop_advertising(void) {
  return bt_le_adv_stop();
}

int sc_ble_set_advertising_data(uint8_t n_presses) {
  RET_CHECK(n_presses <= 7, "n_presses must be <= 7");

  static uint8_t pkt_id = 0;

  // 0xfcd2 - bthome.io service UUID.
  service_data[0] = 0xd2;
  service_data[1] = 0xfc;

  // Service header - no encryption, bt home v2.
  service_data[2] = 0x40;

  // Packet ID.
  service_data[3] = 0x00;
  service_data[4] = pkt_id++;

  // We can only encode up to 5 presses as button actions.
  if (n_presses <= 5) {
    // Button.
    service_data[5] = 0x3a;
    service_data[6] = n_presses;
    service_data[7] = 0x00;
  } else {
    // Dimmer.
    service_data[5] = 0x3c;
    service_data[6] = n_presses - 5;
    service_data[7] = n_presses;
  }

  LOG_HEXDUMP_DBG(ad, sizeof(ad), "Encoded BLE adv: ");

  return 0;
}