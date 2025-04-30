#include "key_event.h"

static int p_esc = 0;
static int long_key = 0;

int use_powerkey_as_esc(int state) {
	int old = p_esc;
	p_esc = state;
	return old;
}

int is_powerkey_as_esc() {
	if (INPUT_KEY_ESC != INPUT_KEY_POWER) {
		return 0;
	}
	return p_esc;
}

int is_key_event_value(int value) {
	if (value == KEY_EVENT_ABORT ||
	    value == KEY_EVENT_OK ||
	    value == KEY_EVENT_DOWN ||
	    value == KEY_EVENT_UP ||
	    value == KEY_EVENT_NEXT ||
	    value == KEY_EVENT_BACK) {
		return 1;
	}

	return 0;
}

int set_support_long_key(int state) {
	int old = long_key;
	long_key = state;
	return old;
}

int is_support_long_key() {
	return long_key;
}