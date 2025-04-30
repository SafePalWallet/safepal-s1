#ifndef _INI_PARSER_H_
#define _INI_PARSER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "map.h"

#ifdef __cplusplus
extern "C" {
#endif

int ini_parser_load(const char *ininame, map_str_t *map);

int ini_parser_trans(map_str_t *src_map, map_str_t *dst_map);

#ifdef __cplusplus
}
#endif

#endif
