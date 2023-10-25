#include "hid.h"

#include <bluetooth/services/hids.h>
#include <sclib/macros.h>
#include <zephyr/logging/log.h>

#define BASE_USB_HID_SPEC_VERSION 0x0101

#define OUTPUT_REPORT_MAX_LEN 1
#define OUTPUT_REPORT_BIT_MASK_CAPS_LOCK 0x02
#define INPUT_REP_KEYS_REF_ID 1
#define OUTPUT_REP_KEYS_REF_ID 1
#define MODIFIER_KEY_POS 0
#define SHIFT_KEY_CODE 0x02
#define SCAN_CODE_POS 2
#define KEYS_MAX_LEN (INPUT_REPORT_KEYS_MAX_LEN - SCAN_CODE_POS)

BT_HIDS_DEF(hids_obj, OUTPUT_REPORT_MAX_LEN, INPUT_REPORT_KEYS_MAX_LEN);

LOG_MODULE_REGISTER(hid, CONFIG_LOG_DEFAULT_LEVEL);

static void hids_boot_kb_outp_rep_handler(struct bt_hids_rep *rep,
                                          struct bt_conn *conn, bool write) {
  LOG_INF("hids_boot_kb_outp_rep_handler");
}

static void hids_outp_rep_handler(struct bt_hids_rep *rep, struct bt_conn *conn,
                                  bool write) {
  LOG_INF("hids_outp_rep_handler");
}

static void hids_pm_evt_handler(enum bt_hids_pm_evt evt, struct bt_conn *conn) {
  LOG_INF("hids_pm_evt_handler");
}

int sc_hid_init(void) {
  struct bt_hids_init_param hids_init_obj = {0};
  struct bt_hids_inp_rep *hids_inp_rep;
  struct bt_hids_outp_feat_rep *hids_outp_rep;

  static const uint8_t report_map[] = {
    0x05,
    0x01, /* Usage Page (Generic Desktop) */
    0x09,
    0x06, /* Usage (Keyboard) */
    0xA1,
    0x01, /* Collection (Application) */

  /* Keys */
#if INPUT_REP_KEYS_REF_ID
    0x85,
    INPUT_REP_KEYS_REF_ID,
#endif
    0x05,
    0x07, /* Usage Page (Key Codes) */
    0x19,
    0xe0, /* Usage Minimum (224) */
    0x29,
    0xe7, /* Usage Maximum (231) */
    0x15,
    0x00, /* Logical Minimum (0) */
    0x25,
    0x01, /* Logical Maximum (1) */
    0x75,
    0x01, /* Report Size (1) */
    0x95,
    0x08, /* Report Count (8) */
    0x81,
    0x02, /* Input (Data, Variable, Absolute) */

    0x95,
    0x01, /* Report Count (1) */
    0x75,
    0x08, /* Report Size (8) */
    0x81,
    0x01, /* Input (Constant) reserved byte(1) */

    0x95,
    0x06, /* Report Count (6) */
    0x75,
    0x08, /* Report Size (8) */
    0x15,
    0x00, /* Logical Minimum (0) */
    0x25,
    0x65, /* Logical Maximum (101) */
    0x05,
    0x07, /* Usage Page (Key codes) */
    0x19,
    0x00, /* Usage Minimum (0) */
    0x29,
    0x65, /* Usage Maximum (101) */
    0x81,
    0x00, /* Input (Data, Array) Key array(6 bytes) */

  /* LED */
#if OUTPUT_REP_KEYS_REF_ID
    0x85,
    OUTPUT_REP_KEYS_REF_ID,
#endif
    0x95,
    0x05, /* Report Count (5) */
    0x75,
    0x01, /* Report Size (1) */
    0x05,
    0x08, /* Usage Page (Page# for LEDs) */
    0x19,
    0x01, /* Usage Minimum (1) */
    0x29,
    0x05, /* Usage Maximum (5) */
    0x91,
    0x02, /* Output (Data, Variable, Absolute), */
          /* Led report */
    0x95,
    0x01, /* Report Count (1) */
    0x75,
    0x03, /* Report Size (3) */
    0x91,
    0x01, /* Output (Data, Variable, Absolute), */
          /* Led report padding */

    0xC0 /* End Collection (Application) */
  };

  hids_init_obj.rep_map.data = report_map;
  hids_init_obj.rep_map.size = sizeof(report_map);

  hids_init_obj.info.bcd_hid = BASE_USB_HID_SPEC_VERSION;
  hids_init_obj.info.b_country_code = 0x00;
  hids_init_obj.info.flags =
      (BT_HIDS_REMOTE_WAKE | BT_HIDS_NORMALLY_CONNECTABLE);

  hids_inp_rep = &hids_init_obj.inp_rep_group_init.reports[INPUT_REP_KEYS_IDX];
  hids_inp_rep->size = INPUT_REPORT_KEYS_MAX_LEN;
  hids_inp_rep->id = INPUT_REP_KEYS_REF_ID;
  hids_init_obj.inp_rep_group_init.cnt++;

  hids_outp_rep =
      &hids_init_obj.outp_rep_group_init.reports[OUTPUT_REP_KEYS_IDX];
  hids_outp_rep->size = OUTPUT_REPORT_MAX_LEN;
  hids_outp_rep->id = OUTPUT_REP_KEYS_REF_ID;
  hids_outp_rep->handler = hids_outp_rep_handler;
  hids_init_obj.outp_rep_group_init.cnt++;

  hids_init_obj.is_kb = true;
  hids_init_obj.boot_kb_outp_rep_handler = hids_boot_kb_outp_rep_handler;
  hids_init_obj.pm_evt_handler = hids_pm_evt_handler;

  RET_IF_ERR(bt_hids_init(&hids_obj, &hids_init_obj));
  return 0;
}

struct bt_hids *sc_hid_get_hids_obj() {
  return &hids_obj;
}
