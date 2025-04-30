
#ifndef WALLET_WIN_H
#define WALLET_WIN_H

#include "minigui_comm.h"

#ifdef __cplusplus
extern "C" {
#endif

void win_init();

const char *win_get_name(int winid);

void win_set_hwnd(int winid, HWND hWnd);

HWND win_get_hwnd(int winid);

int win_show_window(int winid);

int win_hide_window(int winid);

int win_send_message(int winid, int iMsg, WPARAM wParam, LPARAM lParam);

int win_send_notify(int winid, int iMsg, WPARAM wParam, LPARAM lParam);

#ifdef __cplusplus
}
#endif

#endif
