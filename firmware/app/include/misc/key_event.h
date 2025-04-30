#ifndef _KEY_EVENT_H
#define _KEY_EVENT_H

#include <minigui/common.h>

#define MSG_INPUT_KEY (MSG_USER + 500)

#define INPUT_KEY_UP      131
#define INPUT_KEY_DOWN    133
#define INPUT_KEY_LEFT    128
#define INPUT_KEY_RIGHT   129
#define INPUT_KEY_OK      130
#define INPUT_KEY_QR      134
#define INPUT_KEY_POWER   132

#define INPUT_KEY_ESC     INPUT_KEY_POWER //reuse power key

#define INPUT_KEY_MODE 998
#define INPUT_KEY_MENU 997

#define LONG_PRESS_TIME 60 /* unit:10ms */

#define KEY_EVENT_ABORT (-9999)
#define KEY_EVENT_BACK  (-9998) 
#define KEY_EVENT_NEXT  (-9997) 
#define KEY_EVENT_UP    (-9996) 
#define KEY_EVENT_DOWN  (-9995) 
#define KEY_EVENT_OK    (-9994) 

/* KEY MACRO */
enum {
	SHORT_PRESS = 0,
	LONG_PRESS
};

#ifdef __cplusplus
extern "C" {
#endif

int use_powerkey_as_esc(int state);

int is_powerkey_as_esc();

int is_key_event_value(int value);

int set_support_long_key(int state);

int is_support_long_key();

#ifdef __cplusplus
}
#endif
#endif
