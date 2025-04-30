#include "words_util.h"
#include "bip39_english.h"
#include "debug.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static const int gWordListIndexes[26] = {
		//a    b    c    d    e
		136, 253, 439, 551, 651,
		//f   g   h    i    j
		757, 833, 897, 952, 972,
		//k    l   m     n    0
		992, 1068, 1173, 1214, 1269,
		//p    q    r     s    t
		1401, 1409, 1517, 1767, 1888,
		//u   v     w     x                     y     z
		1923, 1969, 2038, RANGE_INVALID_VALUE, 2044, 2048
};

int getWordRange(const char *prefix, Range *result) {
	if (NULL == prefix || NULL == result) {
		db_error("invalid paras prefix:%s result:%p", prefix, result);
		return -1;
	}
	result->startIndex = RANGE_INVALID_VALUE;
	result->endIndex = RANGE_INVALID_VALUE;
	if (!strlen(prefix)) {
		db_error("prefix is empty");
		return 0;
	}
	if (prefix[0] < 'a' || prefix[0] > 'z') {
		db_error("not find in wordlist prefix:%s", prefix);
		return 0;
	}
	int len = strlen(prefix);
	int end = RANGE_INVALID_VALUE;
	int start = RANGE_INVALID_VALUE;

	end = gWordListIndexes[prefix[0] - 'a'] - 1;
	if (end < 0) {
		db_error("not find in wordlist prefix:%s", prefix);
		return 0;
	}
	if (prefix[0] == 'a') {
		start = 0;
	} else {
		start = gWordListIndexes[prefix[0] - 'a' - 1];
		if (start == RANGE_INVALID_VALUE) {
			start = gWordListIndexes[prefix[0] - 'a' - 2];
		}
	}
	//db_msg("%c start range is %d:%s-%d:%s", prefix[0], start, wordlist[start], end, wordlist[end]);
	if (len <= 1) {
		result->startIndex = start;
		result->endIndex = end;
		return end - start + 1;
	}
	int i;
	for (i = start; i <= end; ++i) {
		if (strncmp(prefix + 1, wordlist[i] + 1, len - 1) == 0) {
			if (result->startIndex == RANGE_INVALID_VALUE) {
				result->startIndex = i;
				result->endIndex = i;
			} else {
				result->endIndex = i;
			}
		}
	}
	//db_msg("prefix:%s start range is %d:%s-%d:%s", prefix, result->startIndex, wordlist[result->startIndex], result->endIndex, wordlist[result->endIndex]);
	return getCntOfRange(result);
}

int getCntOfRange(const Range *range) {
	if (NULL == range) {
		db_error("invalid range, range is null");
		return 0;
	}
	if (range->startIndex == RANGE_INVALID_VALUE || range->endIndex == RANGE_INVALID_VALUE) {
		db_error("invalid range value start:%d end:%d", range->startIndex, range->endIndex);
		return 0;
	}
	if (range->endIndex < range->startIndex) {
		db_error("endIndex less startIndex, illegal");
		return 0;
	}
	db_msg("");
	return range->endIndex - range->startIndex + 1;
}

int checkWord(const char *str) {
	if (NULL == str) {
		return 0;
	}
	int i;
	Range result;
	result.startIndex = RANGE_INVALID_VALUE;
	result.endIndex = RANGE_INVALID_VALUE;

	int count = getWordRange(str, &result);
	if (count >= 1) {
		for (i = 0; i < count; ++i) {
			if (strcmp(str, wordlist[result.startIndex + i]) == 0) {
				return 1;
			}
		}
	}
	return 0;
}