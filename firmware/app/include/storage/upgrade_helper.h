
#ifndef WALLET_UPDATEHELPER_H
#define WALLET_UPDATEHELPER_H

#include "minigui_comm.h"

#define UPGRADE_FW_VERIFY_DIGEST_FAILED -4001

#ifdef __cplusplus
extern "C" {
#endif

enum {
	OTA_DISK_STATE_NONE = 0,
	OTA_DISK_STATE_SHARE = 1,
	OTA_DISK_STATE_MOUNT = 2
};

int upgrade_init();

int upgrade_getState();

int upgrade_shareToPC();

int upgrade_unshareFromPC();

int upgrade_umount();

int upgrade_clean();

int upgrade_prepare(HWND hwnd, int upgrade);

int upgrade_do_upgrade(HWND hwnd);

int set_usb_state(int state);

#ifdef __cplusplus
}
#endif
#endif
