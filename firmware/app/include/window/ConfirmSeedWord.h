#ifndef TRUNK_CONFIRMSEEDWORD_H
#define TRUNK_CONFIRMSEEDWORD_H

#include <stdlib.h>
#include <minigui/common.h>
#include "widgets.h"

typedef struct {
	label_string title;
	const uint16_t *seeds;
	uint8_t seedWordCnt;
} ConfirmSeedWordConfig_t;

extern int showConfirmSeedWord(HWND hParent, ConfirmSeedWordConfig_t *config);

#endif
