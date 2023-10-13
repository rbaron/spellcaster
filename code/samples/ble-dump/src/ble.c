#include "ble.h"

#include <bluetooth/services/nus.h>
#include <sclib/macros.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>

#define BT_UUID_CUSTOM_SERVICE_VAL \
  BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef0)

static struct bt_uuid_128 vnd_uuid =
    BT_UUID_INIT_128(BT_UUID_CUSTOM_SERVICE_VAL);

static struct bt_uuid_128 vnd_enc_uuid = BT_UUID_INIT_128(
    BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef1));

static struct bt_uuid_128 vnd_auth_uuid = BT_UUID_INIT_128(
    BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef2));

#define VND_MAX_LEN 20

static uint8_t vnd_value[VND_MAX_LEN + 1] = {'V', 'e', 'n', 'd', 'o', 'r'};
// static uint8_t vnd_auth_value[VND_MAX_LEN + 1] = {'V', 'e', 'n', 'd', 'o',
// 'r'}; static uint8_t vnd_wwr_value[VND_MAX_LEN + 1] = {'V', 'e', 'n', 'd',
// 'o', 'r'};

// TODO: rename.
#define SC_MS_TO_INTERVAL(value_ms) ((uint16_t)(value_ms) / 0.625f)

// LOG_MODULE_REGISTER(ble, CONFIG_LOG_DEFAULT_LEVEL);
LOG_MODULE_REGISTER(ble, LOG_LEVEL_DBG);

static ssize_t read_vnd(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                        void *buf, uint16_t len, uint16_t offset) {
  // const char *value = attr->user_data;
  // return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
  // strlen(value)); Write 0xabab to the buffer.
  return bt_gatt_attr_read(conn, attr, buf, len, offset, "hello",
                           strlen("hello"));
}

static ssize_t write_vnd(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                         const void *buf, uint16_t len, uint16_t offset,
                         uint8_t flags) {
  uint8_t *value = attr->user_data;
  if (offset + len > VND_MAX_LEN) {
    return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
  }
  memcpy(value + offset, buf, len);
  value[offset + len] = 0;
  return len;
}

BT_GATT_SERVICE_DEFINE(
    vnd_svc, BT_GATT_PRIMARY_SERVICE(&vnd_uuid),
    BT_GATT_CHARACTERISTIC(&vnd_enc_uuid.uuid,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE |
                               BT_GATT_CHRC_INDICATE,
                           BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, read_vnd,
                           write_vnd, vnd_value), );

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    // BT_DATA_BYTES(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME),
    // BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_CUSTOM_SERVICE_VAL),
    // BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_NUS_VAL),
};

static const struct bt_data sd[] = {
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_NUS_VAL),
};

/*
  Callbacks.
*/
void mtu_updated(struct bt_conn *conn, uint16_t tx, uint16_t rx) {
  printk("Updated MTU: TX: %d RX: %d bytes\n", tx, rx);
}

static struct bt_gatt_cb gatt_callbacks = {.att_mtu_updated = mtu_updated};

static void connected(struct bt_conn *conn, uint8_t err) {
  if (err) {
    LOG_ERR("Connection failed (err 0x%02x)\n", err);
    return;
    LOG_INF("Connected\n");
  }
}

static void disconnected(struct bt_conn *conn, uint8_t reason) {
  LOG_INF("Disconnected (reason 0x%02x)\n", reason);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
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
  // RET_IF_ERR(bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL,
  // 0));

  RET_IF_ERR(bt_nus_init(/*callbacks=*/NULL));

  RET_IF_ERR(bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), sd,
                             ARRAY_SIZE(sd)));

  // Install callbacks.
  bt_gatt_cb_register(&gatt_callbacks);
  // bt_conn_auth_cb_register(&auth_cb_display);
  return 0;
}

int sc_ble_send(float initial_row_angle, uint8_t *data, uint16_t len) {
  BUILD_ASSERT(sizeof(float) == 4, "sizeof(float) != 4");

  // Send header 0xff, hi_len, lo_len.
  uint8_t header[3 + 4] = {
      // Magic byte.
      0xff,
      // 2 bytes, length of entries in bytes.
      (uint8_t)(len >> 8),
      (uint8_t)(len & 0xff),
      // 4 bytes, initial row angle.
      ((uint8_t *)&initial_row_angle)[0],
      ((uint8_t *)&initial_row_angle)[1],
      ((uint8_t *)&initial_row_angle)[2],
      ((uint8_t *)&initial_row_angle)[3],
  };
  RET_IF_ERR(bt_nus_send(NULL, header, sizeof(header)));

  // Send in chunks of 20 bytes.
  uint8_t *data_ptr = data;
  uint16_t remaining_len = len;
  while (remaining_len > 0) {
    uint16_t chunk_len = MIN(remaining_len, 20);
    RET_IF_ERR(bt_nus_send(NULL, data_ptr, chunk_len));
    data_ptr += chunk_len;
    remaining_len -= chunk_len;
  }

  return 0;
}