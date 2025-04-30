#define LOG_TAG "ini_parser"

#include <ctype.h>
#include <stdarg.h>
#include <unistd.h>
#include "ini_parser.h"
#include "debug.h"

#define ASCIILINESZ         (255)

#define DEFINE_VAL_SECTION "var"

typedef enum {
	LINE_UNPROCESSED,
	LINE_ERROR,
	LINE_EMPTY,
	LINE_COMMENT,
	LINE_SECTION,
	LINE_VALUE,
	LINE_INCLUDE
} line_status;

static const char *strlwc(const char *in, char *out, unsigned len) {
	unsigned i;

	if (in == NULL || out == NULL || len == 0) return NULL;
	i = 0;
	while (in[i] != '\0' && i < len - 1) {
		out[i] = (char) tolower((int) in[i]);
		i++;
	}
	out[i] = '\0';
	return out;
}

static char *xstrdup(const char *s) {
	char *t;
	size_t len;
	if (!s)
		return NULL;

	len = strlen(s) + 1;
	t = (char *) malloc(len);
	if (t) {
		memcpy(t, s, len);
	}
	return t;
}

static unsigned strstrip(char *s) {
	char *last = NULL;
	char *dest = s;

	if (s == NULL) return 0;

	last = s + strlen(s);
	while (isspace((int) *s) && *s) s++;
	while (last > s) {
		if (!isspace((int) *(last - 1)))
			break;
		last--;
	}
	*last = (char) 0;

	memmove(dest, s, last - s + 1);
	return last - s;
}

static line_status ini_parser_line(const char *input_line, char *section, char *key, char *value) {
	line_status sta;
	char *line = NULL;
	size_t len;

	line = xstrdup(input_line);
	len = strstrip(line);
	sta = LINE_UNPROCESSED;
	if (strncmp(line, "#include ", 9) == 0) {
		if (line[9] == '"') {
			strcpy(value, line + 10);
		} else {
			strcpy(value, line + 9);
		}
		len = strlen(value);
		if (len > 0 && value[len - 1] == '"') {
			len--;
			value[len] = 0;
		}
		sta = LINE_INCLUDE;
	} else if (len < 1) {
		/* Empty line */
		sta = LINE_EMPTY;
	} else if (line[0] == '#' || line[0] == ';') {
		/* Comment line */
		sta = LINE_COMMENT;
	} else if (line[0] == '[' && line[len - 1] == ']') {
		/* Section name */
		sscanf(line, "[%[^]]", section);
		strstrip(section);
		strlwc(section, section, len);
		sta = LINE_SECTION;
	} else if (sscanf(line, "%[^=] = \"%[^\"]\"", key, value) == 2
	           || sscanf(line, "%[^=] = '%[^\']'", key, value) == 2) {
		/* Usual key=value with quotes, with or without comments */
		strstrip(key);
		strlwc(key, key, len);
		/* Don't strip spaces from values surrounded with quotes */
		sta = LINE_VALUE;
	} else if (sscanf(line, "%[^=] = %[^;#]", key, value) == 2) {
		/* Usual key=value without quotes, with or without comments */
		strstrip(key);
		strlwc(key, key, len);
		strstrip(value);
		/*
		 * sscanf cannot handle '' or "" as empty values
		 * this is done here
		 */
		if (!strcmp(value, "\"\"") || (!strcmp(value, "''"))) {
			value[0] = 0;
		}
		sta = LINE_VALUE;
	} else if (sscanf(line, "%[^=] = %[;#]", key, value) == 2
	           || sscanf(line, "%[^=] %[=]", key, value) == 2) {
		/*
		 * Special cases:
		 * key=
		 * key=;
		 * key=#
		 */
		strstrip(key);
		strlwc(key, key, len);
		value[0] = 0;
		sta = LINE_VALUE;
	} else {
		/* Generate syntax error */
		sta = LINE_ERROR;
	}

	free(line);
	return sta;
}

static int save_value_to_tmp_map(map_str_t *map, const char *section, const char *key, char *value) {
	char fullkey[128];
	snprintf(fullkey, sizeof(fullkey), "%s:%s", section, key);
	//db_msg("set %s -> %s", fullkey, value);
	return map_set_str(map, fullkey, value);
}

static int get_final_value(map_str_t *src_map, map_str_t *dst_map, const char *key, char *newval, size_t size_newval) {
	char fullkey[128];
	char tmpval[ASCIILINESZ + 1];
	char *p0, *p1, *pk, *sec_p0;
	const char *sec_p1;
	int total;
	const char *value = map_get_str(dst_map, key);
	if (value) {
		strncpy(newval, value, size_newval);
		return 0;
	}
	value = map_get_str(src_map, key);
	if (!value) {
		return -1;
	}
	p0 = strchr(value, '{');
	if (!p0) {
		strncpy(newval, value, size_newval);
		return 0;
	}

	memset(newval, 0, size_newval);
	total = p0 - value;
	if (total) {
		strncpy(newval, value, total);
	}
	p0++;
	p1 = newval + total;
	int state = 1;
	pk = p0;
	while (*p0) {
		if (state == 0) {
			if (*p0 == '{') {
				state = 1;
				pk = p0 + 1;
			} else {
				*p1++ = *p0;
				if (++total >= ASCIILINESZ) {
					break;
				}
			}
		} else {
			if (*p0 == '}') {
				if (pk) {
					if (p0 > pk) {
						*p0 = '\0';
						if (pk[0] == ':') {
							sec_p0 = tmpval;
							sec_p1 = key;
							while (*sec_p1 && *sec_p1 != ':') {
								*sec_p0++ = *sec_p1++;
							}
							*sec_p0 = 0;
							snprintf(fullkey, sizeof(fullkey), "%s%s", tmpval, pk);
						} else if (strchr(pk, ':') == NULL) {
							snprintf(fullkey, sizeof(fullkey), "%s:%s", DEFINE_VAL_SECTION, pk);
						} else {
							snprintf(fullkey, sizeof(fullkey), "%s", pk);
						}
						if (get_final_value(src_map, dst_map, fullkey, tmpval, ASCIILINESZ) >= 0) {
							total += strlen(tmpval);
							if (total > ASCIILINESZ) {
								*p0 = '}'; //reset
								break;
							}
							strcpy(p1, tmpval);
							p1 = newval + total;
						}
						*p0 = '}'; //reset
					}
					pk = NULL;
				}
				state = 0;
			}
		}
		p0++;
	}
	return 0;
}

int ini_parser_load(const char *ininame, map_str_t *map) {
	FILE *in;
	char line[ASCIILINESZ + 1];
	char section[ASCIILINESZ + 1];
	char key[ASCIILINESZ + 1];
	char val[ASCIILINESZ + 1];
	char *p;
	int last = 0;
	int len;
	int lineno = 0;
	int errs = 0;
	int mem_err = 0;

	if ((in = fopen(ininame, "r")) == NULL) {
		db_error("cannot open %s", ininame);
		return -1;
	}

	memset(line, 0, ASCIILINESZ);
	memset(section, 0, ASCIILINESZ);
	memset(key, 0, ASCIILINESZ);
	memset(val, 0, ASCIILINESZ);
	last = 0;

	while (fgets(line + last, ASCIILINESZ - last, in) != NULL) {
		lineno++;
		len = (int) strlen(line) - 1;
		if (len <= 0)
			continue;
		/* Safety check against buffer overflows */
		if (line[len] != '\n' && !feof(in)) {
			db_error("input line too long in %s (%d)", ininame, lineno);
			map_clean(map);
			fclose(in);
			return -1;
		}
		/* Get rid of \n and spaces at end of line */
		while ((len >= 0) &&
		       ((line[len] == '\n') || (isspace(line[len])))) {
			line[len] = 0;
			len--;
		}
		if (len < 0) { /* Line was entirely \n and/or spaces */
			len = 0;
		}
		/* Detect multi-line */
		if (line[len] == '\\') {
			/* Multi-line value */
			last = len;
			continue;
		} else {
			last = 0;
		}
		switch (ini_parser_line(line, section, key, val)) {
			case LINE_EMPTY:
			case LINE_COMMENT:
			case LINE_SECTION:
				break;
			case LINE_VALUE:
				mem_err = save_value_to_tmp_map(map, section, key, val);
				break;
			case LINE_ERROR:
				db_error("syntax error in %s (%d):-> %s", ininame, lineno, line);
				errs++;
				break;
			case LINE_INCLUDE:
				if (strlen(ininame) + strlen(val) < ASCIILINESZ) {
					strcpy(key, ininame);
					p = strrchr(key, '/');
					if (p) *(p + 1) = 0;
					strcat(key, val);
					if (access(key, F_OK) == 0) {
						db_msg("include file:%s", key);
						ini_parser_load(key, map);
					} else {
						db_error("include file:%s but not exist", key);
					}
				}
			default:
				break;
		}
		memset(line, 0, ASCIILINESZ);
		last = 0;
		if (mem_err < 0) {
			ALOGE("memory allocation failure");
			break;
		}
	}
	fclose(in);
	return map_size(map);
}

int ini_parser_trans(map_str_t *src_map, map_str_t *dst_map) {
	const char *key;
	const char *value;
	char newval[ASCIILINESZ + 1];
	newval[ASCIILINESZ] = 0;
	map_iter_t iter = map_iter(src_map);
	while ((key = map_next(src_map, &iter))) {
		value = map_get_str(src_map, key);
		if (value) {
			if (strchr(value, '{') == NULL) {
				//db_msg("set %s -> %s", key, value);
				map_set_str(dst_map, key, value);
			} else {
				if (get_final_value(src_map, dst_map, key, newval, ASCIILINESZ) >= 0) {
					//db_msg("set %s -> %s", key, newval);
					map_set_str(dst_map, key, newval);
				}
			}
		}
	}
	return 0;
}
