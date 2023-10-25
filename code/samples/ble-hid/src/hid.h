#ifndef _SC_HID_H_
#define _SC_HID_H_

// Max keys pressed at the same time.
#define KEY_PRESS_MAX 6
// Number of bytes in key report.
#define INPUT_REPORT_KEYS_MAX_LEN (1 + 1 + KEY_PRESS_MAX)

enum { OUTPUT_REP_KEYS_IDX = 0 };

enum { INPUT_REP_KEYS_IDX = 0 };

int sc_hid_init(void);

struct bt_hids *sc_hid_get_hids_obj();

#endif  // _SC_HID_H_