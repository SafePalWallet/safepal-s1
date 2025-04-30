#ifndef TRUNK_BACKUPSEEDWORD_H
#define TRUNK_BACKUPSEEDWORD_H

#include <stdlib.h>
#include <stdint.h>
#include <minigui/common.h>
#include "widgets.h"

typedef struct {
	label_string title;
	const uint16_t *seeds;
	uint8_t seedWordCnt;
} BackupSeedWordConfig_t;

extern int showBackupSeedWord(HWND hParent, BackupSeedWordConfig_t *config);

#endif
