#ifndef WALLET_LOADING_WIN_H
#define WALLET_LOADING_WIN_H

#ifdef __cplusplus
extern "C" {
#endif

extern int loading_win_processing;

int loading_win_start(unsigned int hwnd, const char *title, const char *detail, int style);

void loading_win_stop();

int loading_win_refresh();

int loading_win_refresh_timeout(int time_ms);

#ifdef __cplusplus
}
#endif
#endif
