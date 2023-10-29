#ifndef _SC_BUTTON_H_
#define _SC_BUTTON_H_

#include <stdbool.h>

typedef enum {
  SC_BUTTON_A,
  SC_BUTTON_B,
  SC_BUTTON_NONE,
} sc_button_t;

typedef enum {
  SC_BUTTON_EVENT_SHORT_PRESS,
  SC_BUTTON_EVENT_LONG_PRESS,
  SC_BUTTON_EVENT_DOUBLE_PRESS,
  SC_BUTTON_EVENT_TRIPLE_PRESS,
  SC_BUTTON_EVENT_QUADRUPLE_PRESS,
  SC_BUTTON_EVENT_QUINTUPLE_PRESS,
  SC_BUTTON_EVENT_SEXTUPLE_PRESS,
  SC_BUTTON_EVENT_NONE,
} sc_button_event_t;

typedef void (*sc_button_callback_t)(sc_button_t button,
                                     sc_button_event_t event);

// Inits button driver.
int sc_button_init();

// Configures ISR and calls callback on debounced button press/release.
int sc_button_register_callback(sc_button_callback_t callback);

// Returns:
// 1 if button is active
// 0 if button is inactive
// -1 on error
int sc_button_poll(sc_button_t sc_button);

#endif  // _SC_BUTTON_H_
