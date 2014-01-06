#pragma once
#include <stdio.h>
#include "types.h"

typedef enum {
	HTTP, LOCAL
} SoundFileType;

struct sound_file_info {
	const char *url;
	int fd;
	size_t size;
	size_t offset;
	size_t mp3_data_start;
	size_t mp3_data_end;
	SoundFileType type;
};

struct sound_file_info *sound_file_open(const char *URL);
int sound_file_close(struct sound_file_info *file);
int sound_file_load(struct sound_file_info *file, u8 *buff, size_t size);
int sound_file_seek(struct sound_file_info *file, size_t offset);
