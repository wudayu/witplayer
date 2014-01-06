#pragma once
#include "sound_file.h"

struct sound_file_info *local_file_open(const char *URL);
int local_file_close(struct sound_file_info *file);
int local_file_load(struct sound_file_info *file, u8 *buff, size_t size);
