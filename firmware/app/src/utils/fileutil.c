#define LOG_TAG "util"

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include "fileutil.h"
#include "debug.h"

int file_get_int(const char *pathname) {
	int fd;
	char linebuf[16] = {0};
	fd = open(pathname, O_RDONLY);
	if (fd <= 0) {
		db_msg("open %s false", pathname);
		return -1;
	}

	if (read(fd, linebuf, sizeof(linebuf)) > 0) {
		close(fd);
		return atoi(linebuf);
	}
	close(fd);
	return -1;
}

int file_get_line(const char *pathname, char *buffer, int length) {
	FILE *fp;
	fp = fopen(pathname, "r");
	if (fp == NULL) {
		return -1;
	}
	if (fgets(buffer, length, fp) != NULL) {
		fclose(fp);
		return strlen(buffer);
	}
	fclose(fp);
	return 0;
}

int file_get_contents(const char *pathname, unsigned char *buffer, int length) {
	int fd;
	fd = open(pathname, O_RDONLY);
	if (fd <= 0) {
		db_msg("open %s false", pathname);
		return -1;
	}
	int ret;
	int readed = 0;
	while (readed < length) {
		ret = read(fd, buffer + readed, length - readed);
		if (ret > 0) {
			readed += ret;
		} else {
			break;
		}
	}
	close(fd);
	return readed;
}

long file_size(const char *path) {
	long fsize;
	FILE *fp = fopen(path, "rb");
	if (fp == NULL) return -1;
	fseek(fp, 0, SEEK_END);
	fsize = ftell(fp);
	fclose(fp);
	return fsize;
}

int file_put_contents(const char *pathname, const void *data, int size) {
	int fd = open(pathname, O_WRONLY);
	if (fd <= 0) {
		db_serr("open:%s failed", pathname);
		return -1;
	}
	int datalen = write(fd, data, size);
	close(fd);
	if (datalen != size) {
		db_serr("write %s to %s fasle ", (const char *) data, pathname);
		return -2;
	}
	close(fd);
	return 0;
}

int file_write_int(const char *pathname, uint32_t n) {
	char tmp[20] = {0};
	int size = snprintf(tmp, sizeof(tmp), "%u", n);
	return file_put_contents(pathname, tmp, size);
}
