#define LOG_TAG "show_bmp"

#include <unistd.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <linux/kd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <string.h>
#include <common_c.h>

#define FRAMEBUFFER "/dev/fb0"

static unsigned char *frameBuffer = 0;

typedef struct {
	char cfType[2];
	int cfSize;
	int cfReserved;
	int cfoffBits;
}__attribute__((packed)) BITMAPFILEHEADER;

typedef struct {
	char ciSize[4];
	int ciWidth;
	int ciHeight;
	char ciPlanes[2];
	int ciBitCount;
	char ciCompress[4];
	char ciSizeImage[4];
	char ciXPelsPerMeter[4];
	char ciYPelsPerMeter[4];
	char ciClrUsed[4]; 
	char ciClrImportant[4];
}__attribute__((packed)) BITMAPINFOHEADER;

typedef struct {
	unsigned char blue;
	unsigned char green;
	unsigned char red;
	//unsigned char reserved;
}__attribute__((packed)) PIXEL;

static int show_bmp(const char *bmp_file, int screen_w, int screen_h) {
	BITMAPFILEHEADER FileHead;
	BITMAPINFOHEADER InfoHead;
	FILE *fp;
	int rc;
	int line_x, line_y;
	int location = 0, BytesPerLine = 0;
	fp = fopen(bmp_file, "rb");
	if (fp == NULL) {
		ALOGE("open:%s false", bmp_file);
		return (-1);
	}
	rc = fread(&FileHead, sizeof(BITMAPFILEHEADER), 1, fp);
	if (rc != 1) {
		ALOGE("read header error!");
		fclose(fp);
		return (-2);
	}
	if (memcmp(FileHead.cfType, "BM", 2) != 0) {
		ALOGE("it's not a BMP file");
		fclose(fp);
		return (-3);
	}

	rc = fread((char *) &InfoHead, sizeof(BITMAPINFOHEADER), 1, fp);
	if (rc != 1) {
		ALOGE("read infoheader error!");
		fclose(fp);
		return (-4);
	}
	int width = InfoHead.ciWidth;
	int height = InfoHead.ciHeight;
	if (height < 0) height = -height;
	BytesPerLine = screen_w * 4;
	int offset_x = (screen_w - width) / 2;
	int offset_y = (screen_h - height) / 2;

	//ALOGD("width:%d height:%d BytesPerLine:%d offset_x:%d offset_y:%d", width, height, BytesPerLine, offset_x, offset_y);
	if (width > screen_w || height > screen_h) {
		ALOGE("width:%d screen_w:%d height:%d screen_h:%d", width, screen_w, height, screen_h);
		fclose(fp);
		return (-5);
	}
	fseek(fp, FileHead.cfoffBits, SEEK_SET);
	line_x = 0;
	line_y = offset_y;
	unsigned char *f;
	unsigned int *fi = (unsigned int *) frameBuffer;
	unsigned int *end = fi + offset_y * screen_w;
	while (fi < end) {
		*fi++ = 0xff000000;
	}
	fi = (unsigned int *) (frameBuffer + (offset_y + height) * BytesPerLine);
	end = fi + (screen_h - offset_y - height) * screen_w;
	while (fi < end) {
		*fi++ = 0xff000000;
	}

	int readn = 0;
	while (!feof(fp)) {
		PIXEL pix;
		rc = fread((char *) &pix, 1, sizeof(PIXEL), fp);
		readn += rc;
		if (rc != sizeof(PIXEL)) {
			ALOGE("read ends rc:%d", rc);
			break;
		}
		if (line_x == 0) {
			fi = (unsigned int *) (frameBuffer + line_y * BytesPerLine);
			end = fi + offset_x;
			while (fi < end) {
				*fi++ = 0xff000000;
			}
			fi += width;
			end = fi + (screen_w - offset_x - width);
			while (fi < end) {
				*fi++ = 0xff000000;
			}
		}

		f = frameBuffer + line_y * BytesPerLine + (offset_x + line_x) * 4;
		//ALOGD("line_x:%d line_y:%d off:%d\n", line_x, line_y, location);
		*f++ = pix.blue;
		*f++ = pix.green;
		*f++ = pix.red;
		*f = 0xff;//pix.reserved;
		line_x++;
		if (line_x == width) {
			line_x = 0;
			line_y++;
			//ALOGD("line_x:%d line_y:%d\n", line_x, line_y);
			if (line_y == (offset_y + height)) {
				break;
			}
		}
	}
	fclose(fp);
	//ALOGD("end read:%d", readn);
	return (0);
}

int show_shutdown_logo(const char *bmp_file) {
	int screensize = 0;
	struct fb_fix_screeninfo finfo;
	struct fb_var_screeninfo vinfo;
	ALOGD("logo:%s", bmp_file);
	int fbFd = open(FRAMEBUFFER, O_RDWR);
	if (fbFd == -1) {
		ALOGE("Error: cannot open framebuffer device");
		return -1;
	}
	if (ioctl(fbFd, FBIOGET_FSCREENINFO, &finfo) == -1) {
		ALOGE("Error reading fixed information");
		close(fbFd);
		return -2;
	}
	if (ioctl(fbFd, FBIOGET_VSCREENINFO, &vinfo) == -1) {
		ALOGE("Error reading variable information");
		close(fbFd);
		return -3;
	}
	screensize = finfo.smem_len;
	ALOGD("screensize:%d xres:%d yres:%d\n", screensize, vinfo.xres, vinfo.yres);
	frameBuffer = (unsigned char *) mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbFd, 0);
	if (frameBuffer == MAP_FAILED) {
		ALOGE("Error: Failed to map framebuffer device to memory\n");
		close(fbFd);
		return -4;
	}
	//memset(frameBuffer, 0xff000000, screensize);
	show_bmp(bmp_file, vinfo.xres, vinfo.yres);
	
	vinfo.yoffset = 0;
	vinfo.activate = FB_ACTIVATE_NOW;
	ioctl(fbFd, FBIOPAN_DISPLAY, &vinfo);

	munmap(frameBuffer, screensize);
	close(fbFd);
	return 0;
}
