#pragma once

#include "sound_file.h"

#define BUFF_LEN 100 * 1024
#define RQU_LEN 256
#define SERVER_PORT 80
#define HTTP_HEAD_LEN 512
#define THREAD_NUM 4
#define SERVER_PORT 80
#define SEC 5

struct sound_file_info *http_file_open(const char *URL);
int http_file_close(struct sound_file_info *file);
int http_file_load(struct sound_file_info *file, u8 *buff, size_t size);

struct download_arg {
	struct sound_file_info *file;
	unsigned char *buff;
	size_t size;
	size_t offset;
	char *ip;
	size_t ret_size;
};
