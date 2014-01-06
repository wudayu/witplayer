#pragma once

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include "mp3.h"

typedef struct {
	char b;
	char g;
	char r;
} color_t;

struct window {
	int x, y;
	int w, h;
};

struct progressbar_win {
	struct window win;
	color_t bar_color;
	color_t back_color;
	size_t max;
	size_t min;
	size_t cur;
};

struct text_win {
	struct window win;
	color_t font_color;
	color_t back_color;
	int font_size;
	char *text;
};

struct window_info {
	int fd;
	void *vm;

	int width;
	int height;
	int bpp;

	struct fb_fix_screeninfo fix;
	struct fb_var_screeninfo var;

	struct progressbar_win progressbar;
	struct text_win text;
	struct window icon_win;
	struct window wave_win;

	struct timeval total;
	u8 *lrc;
	size_t lrc_size;
	u8 *icon;
	size_t icon_size;
	struct mp3_param *param;
};


#define FB_DEV "/dev/fb0"
#define LEN_W(a)  ((a) * 8 / 10)
#define LSK_H(b)  ((b) * 9 * 4 / 10)

void *get_vm();
struct fb_fix_screeninfo *get_fix();
struct fb_var_screeninfo *get_var();

struct window_info *window_init();
int window_destroy();

int show_wave(struct window *win, u8 *raw_data, size_t size, struct mp3_param *param);
int show_icon(struct window *win, u8 *icon, size_t size);
int show_progressbar(struct progressbar_win *bar);
int show_text(struct text_win *text);
int flush_window(struct timeval tv, void *raw, int size);
