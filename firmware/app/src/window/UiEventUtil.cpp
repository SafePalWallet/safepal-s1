#include "GuiInterface.h"

static int UIEVENT2NETCMD_MAP[] = {
		0,
		CTRL_CMD_SCREEN_SLEEP,
		CTRL_CMD_SET_LANGUAGE,
		CTRL_CMD_SET_AUTO_SHOTDOWN_TIME,
		CTRL_CMD_SET_BRIGHTNESS,
        CTRL_CMD_SET_RAND_PIN_KEYPAD,
        CTRL_CMD_SET_BTC_MULTI_ADDRESS,
		0,
};

int UI_EVENT_TO_NET_CMD(int event) {
	if (event > UI_EVENT_MAP_NETCMD_START && event < UI_EVENT_MAP_NETCMD_END) {
		return UIEVENT2NETCMD_MAP[event];
	}
	return 0;
}

int CMD_TO_UI_EVENT(int cmd) {
	if (cmd > CMD_MIN_VAL && cmd < CMD_MAX_VAL) {
		for (int i = UI_EVENT_MAP_NETCMD_START; i < UI_EVENT_MAP_NETCMD_END; i++) {
			if (UIEVENT2NETCMD_MAP[i] == cmd) {
				return i;
			}
		}
	}
	return 0;
}
