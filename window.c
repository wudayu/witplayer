#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>
#include "window.h"
#include "icon.h"
#include "lrc.h"

static struct window_info windows;

static int set_color(u8 *pixel, int size, color_t color)
{
	switch (size) {
	case 3:
	case 4:
		pixel[0] = color.b;
		pixel[1] = color.g;
		pixel[2] = color.r;
		break;

	case 2:
		break;
	}

	return 0;
}

struct window_info * window_init()
{
	int fd;
	color_t r = {.r = 0xFF};
	color_t g = {.g = 0xFF};
	// color_t b = {.b = 0xFF};

	fd = open(FB_DEV, O_RDWR);

	ioctl(fd, FBIOGET_VSCREENINFO, &windows.var);
	ioctl(fd, FBIOGET_FSCREENINFO, &windows.fix);

	printf("mem_len = %u, line_len = %u\n",
			windows.fix.smem_len, windows.fix.line_length);

	printf("w = %d, h = %d, bpp = %d\n",
			windows.var.xres_virtual, windows.var.yres_virtual, windows.var.bits_per_pixel);

	windows.vm = mmap(NULL, windows.fix.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	memset(windows.vm, 0, windows.fix.smem_len);

	windows.bpp = (windows.var.bits_per_pixel + 7) / 8;
	windows.width = windows.fix.line_length / windows.bpp;
	windows.height = windows.var.yres_virtual;

	windows.fd = fd;

	windows.icon_win.x = 0;
	windows.icon_win.y = 0;
	windows.icon_win.h = 200;
	windows.icon_win.w = 300;

	windows.wave_win.x = 300;
	windows.wave_win.y = 0;
	windows.wave_win.w = 300;
	windows.wave_win.h = 200;

	windows.progressbar.win.x = 300;
	windows.progressbar.win.y = 200;
	windows.progressbar.win.w = 300;
	windows.progressbar.win.h = 20;
	windows.progressbar.bar_color = r;
	windows.progressbar.back_color = g;
	windows.progressbar.min = 0;
	windows.progressbar.max = 0;

	windows.text.win.x = 200;
	windows.text.win.y = 120;
	windows.text.win.w = 200;
	windows.text.win.h = 40;

	return &windows;
}

int window_destroy()
{
	munmap(windows.vm, windows.fix.smem_len);
	close(windows.fd);

	return 0;
}

void *get_vm()
{
	return windows.vm;
}

struct fb_fix_screeninfo *get_fix()
{
	return &windows.fix;
}

struct fb_var_screeninfo *get_var()
{
	return &windows.var;
}

int show_histogram(struct window *win, int x, int percent)
{
	u8 *vm;
	int i;
	color_t black = {.g = 0xFF};
	color_t g = {.r = 0xA0, .b = 0xA0};
	int bpp;

	bpp = windows.bpp;
	vm = get_vm();

	for (i = 0; i < win->h; i++) {
		if (i > (percent * win->h / 100))
			set_color(vm + ((i + win->y) * windows.width + x + win->x) * bpp, bpp, g);
		else
			set_color(vm + ((i + win->y) * windows.width + x + win->x) * bpp, bpp, black);
	}

	return 0;
}

int show_wave(struct window *win, u8 *raw_data, size_t size, struct mp3_param *param)
{
	int frames;
	int frame_size;
	int i, j;
	int bpp;
	u8 *vm;

	frame_size = param->channels * (param->bits_per_sample + 7) / 8;
	frames = size / frame_size;

	vm = get_vm();
	bpp = windows.bpp;

	i = 0;
	if (frames < win->w) {
		int rest;

		rest = win->w - frames;

		for (; i < rest; i++) {
			for (j = 0; j < win->h; j++) {
				memcpy(vm + ((j + win->y) * windows.width + i + win->x) * bpp,
						vm + ((j + win->y) * windows.width + i + win->x + win->w - rest) * bpp, bpp);
			}
		}

	}

	for (j = 0; j < frames && (i + j) < win->w; j++) {
		int val = 0;

		val = *(u16 *)(raw_data + j * frame_size) * 100 >> 16;
		show_histogram(win, i + j, val);
	}

	return 0;
}

int show_icon(struct window *win, u8 *icon, size_t size)
{
	int ret;

	ret = show_icon_coord(win, icon, size);

	return ret;
}

static void draw_pixel(int x, int y, color_t color)
{
	u8 *vm;
	int w;
	int bpp;

	vm = get_vm();
	w = windows.width;
	bpp = windows.bpp;

	set_color(vm + (y * w + x) * bpp, bpp, color);
}

int show_progressbar(struct progressbar_win *bar)
{
	int i, j;
	size_t current = bar->cur;
	size_t total = bar->max;

	for (i = 0; i < bar->win.h; i++) {
		for (j = 0; j < bar->win.w; j++) {
			if (j < current * bar->win.w / total)
				draw_pixel(j + bar->win.x, i + bar->win.y, bar->bar_color);
			else
				draw_pixel(j + bar->win.x, i + bar->win.y, bar->back_color);
		}
	}

	return 0;
}

void draw_line(int y, color_t color)
{
	int w;
	int i;

	w = windows.width;

	for (i = 0; i < w; i++) {
		draw_pixel(i, y, color);
	}
}

int flush_window(struct timeval tv, void *raw, int size)
{
	windows.progressbar.cur = tv.tv_sec * 1000 + tv.tv_usec / 1000;
	windows.progressbar.max = windows.total.tv_sec * 1000 + tv.tv_usec / 1000;

	show_icon(&windows.icon_win, windows.icon, windows.icon_size);
	show_progressbar(&windows.progressbar);
	show_lyric(windows.lrc, windows.lrc_size, &windows.total, &tv);
	show_wave(&windows.wave_win, raw, size, windows.param);

	return 0;
}
