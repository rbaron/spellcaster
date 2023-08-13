#ifndef _SC_BUTTON_H_
#define _SC_BUTTON_H_

#include <stdbool.h>

typedef enum {
  SC_BUTTON_SW1 = 0,
} sc_button_t;

typedef void (*sc_button_callback_t)(sc_button_t button, bool is_active);

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
