#include <stdlib.h>
#include <string.h>
#include "sound_file.h"
#include "http.h"
#include "local.h"

struct sound_file_info *sound_file_open(const char *URL)
{
	struct sound_file_info *file;

	if (!memcmp(URL, "http://", 7)) {
		file = http_file_open(URL);
	} else {
		file = local_file_open(URL);
	}

	return file;
}

int sound_file_close(struct sound_file_info *file)
{
	if (file->type == LOCAL) {
		local_file_close(file);
	} else if (file->type == HTTP) {
		http_file_close(file);
	}

	free(file);

	return 0;
}

int sound_file_load(struct sound_file_info *file, u8 *buff, size_t size)
{
	int ret = -1;

	if (file->type == LOCAL) {
		ret = local_file_load(file, buff, size);
	} else if (file->type == HTTP) {
		ret = http_file_load(file, buff, size);
	}

	if (ret > 0)
		file->offset += ret;

	return ret;
}

int sound_file_seek(struct sound_file_info *file, size_t offset)
{
	file->offset = offset;

	return 0;
}
