#include <png.h>
#include <error.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include "icon.h"

int show_icon_coord(struct window *png_win, u8 *icon, size_t size)
{
	FILE *fp;
	int ret;
	int i, j;
	int png_fd;
	int png_bpp;
	int png_llen;
	int png_width;
	int png_height;
	int png_color_type;
	png_structp png_ptr;
	png_infop png_end_ptr;
	png_infop png_start_ptr;
	png_bytep *png_rows;
	u8 *pic_start;
	u8 *png_pdata;
	u8 png_data[DATA_BUFF_SIZE];

	/* first, find out where is the start of the pic */
	if (icon && (pic_start = (u8 *)memmem(icon, size, PNG_MAGIC, strlen(PNG_MAGIC)))) {
		--pic_start;
		/* write the icon png_data into the tmp file, ready to use fopen() */
		png_fd = open(TMP_FILE, O_RDWR | O_CREAT, 0666);
		if (png_fd < 0) {
			perror("open png or create tmp file failed!\n");
			return -1;
		}

		ret = write(png_fd, pic_start, size - (pic_start - icon));
		if (ret < 0) {
			perror("write the tmp file error!\n");
			close(png_fd);
			return ret;
		}

		close(png_fd);

		/* use fopen to initialize fp */
		if ((fp = fopen(TMP_FILE, "rb")) == NULL) {
			perror("fopen");
			return -1;
		}
	} else {
		/* use fopen to initialize fp */
		if ((fp = fopen(DEF_FILE, "rb")) == NULL) {
			perror("fopen");
			return -1;
		}
	}

	/* create info */
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (NULL == png_ptr) {
		fprintf(stderr,"create png_ptr error!\n");
		ret = -1;
		goto L1;
	}

	png_start_ptr = png_create_info_struct(png_ptr);
	if (NULL == png_start_ptr) {
		fprintf(stderr, "create png_start_ptr error!\n");
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		ret = -1;
		goto L1;
	}

	png_end_ptr = png_create_info_struct(png_ptr);
	if (NULL == png_end_ptr) {
		fprintf(stderr, "create png_end_ptr error!\n");
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		ret = -1;
		goto L1;
	}

	/* read info chunks */
	png_init_io(png_ptr, fp);

	png_read_info(png_ptr, png_start_ptr);

	/* read header */
	png_get_IHDR(png_ptr, png_start_ptr, (png_uint_32*)&png_width, (png_uint_32*)&png_height,
			&png_bpp, &png_color_type, NULL, NULL, NULL);

	/* read image png_data */
	png_llen = png_get_rowbytes(png_ptr, png_start_ptr);

	png_rows = (png_bytep *)malloc(png_height * sizeof(char *));
	for (i = 0; i < png_height; i++) {
		png_rows[i] = (png_bytep) (png_data + png_llen * i);
	}

	png_read_image(png_ptr, png_rows);

	free(png_rows);

	png_read_end(png_ptr, png_start_ptr);

	png_destroy_read_struct(&png_ptr, &png_start_ptr, &png_end_ptr);

	struct fb_fix_screeninfo *fb_fix;
	struct fb_var_screeninfo *fb_var;

	if (NULL == (fb_var = get_var())) {
		perror("Var error");
		ret = -1;
		goto L1;
	}

	if (NULL == (fb_fix = get_fix())) {
		perror("Fix error");
		ret = -1;
		goto L1;
	}

	struct screen screen_info;
	screen_info.bpp = (fb_var->bits_per_pixel + 7) >> 3;
	screen_info.width = fb_fix->line_length / (screen_info.bpp);
	screen_info.height = fb_var->yres_virtual;
	screen_info.frame_size = screen_info.width * screen_info.height * screen_info.bpp;

	struct color *mem_start, *mem_p, *mem_pline;

	mem_start = (struct color *)get_vm();

	if (NULL == mem_start) {
		perror("Mmap fb memmory");
		return -1;
	}

	png_pdata = png_data;

	mem_pline = mem_start + png_win->y * screen_info.width;

	switch (png_color_type) {
		case PNG_COLOR_TYPE_RGB:
			for (i = 0; i < png_height && i < png_win->h; ++i) {
				mem_p = mem_pline + png_win->x;
				for (j = 0; j < png_width && j < png_win->w; ++j, ++mem_p) {
					mem_p->blue = *png_pdata++;
					mem_p->green = *png_pdata++;
					mem_p->red = *png_pdata++;
					mem_p->alpha = 0xff;
				}

				png_pdata += (png_width - j) * 3;
				mem_pline += screen_info.width;
			}
			break;

		case PNG_COLOR_TYPE_RGB_ALPHA:
			for (i = 0; i < png_height && i < png_win->h; ++i) {
				mem_p = mem_pline + png_win->x;
				for (j = 0; j < png_width && j < png_win->w; ++j, ++mem_p) {
					mem_p->blue = *png_pdata++;
					mem_p->green = *png_pdata++;
					mem_p->red = *png_pdata++;
					mem_p->alpha = *png_pdata++;
				}

				png_pdata += (png_width - j) * 4;
				mem_pline += screen_info.width;
			}
			break;

		case PNG_COLOR_TYPE_GRAY_ALPHA:
			for (i = 0; i < png_height && i < png_win->h; ++i) {
				mem_p = mem_pline + png_win->x;
				for (j = 0; j < png_width && j < png_win->w; ++j, ++mem_p)
					mem_p->alpha = *png_pdata++;

				png_pdata += png_width - j;
				mem_pline += screen_info.width;
			}
			break;
	}

L1:
	fclose(fp);

	return ret;
}

