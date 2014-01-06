#pragma once

#include "window.h"
#include "types.h"

#define DATA_BUFF_SIZE (2 * 1024 * 1024)
#define PLT_BUFF_SIZE (1024 * 1024)
#define FB_DEV "/dev/fb0"
#define PNG_MAGIC "PNG"
#define TMP_FILE "/tmp/tmp.png"
#define DEF_FILE "nopic.png"

struct screen {
	int width;
	int height;
	int bpp;
	int frame_size;
};

struct color {
	u8 red;
	u8 green;
	u8 blue;
	u8 alpha;
};

int show_icon_coord(struct window *, u8 *, size_t);
//fixme
void *memmem(const void *, size_t, const void *, size_t);
