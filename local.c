#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "sound_file.h"
#include "local.h"

struct sound_file_info *local_file_open(const char *URL)
{
	int fd;
	int ret;
	struct stat sa;
	struct sound_file_info *info;

	info = malloc(sizeof(* info));

	fd = open(URL, O_RDONLY);
	if (fd < 0) {
		perror("local open");
		return NULL;
	}

	ret = fstat(fd, &sa);
	if (ret < 0) {
		fprintf(stderr, "local file get size failed!");
		close(fd);
		return NULL;
	}

	info->size = sa.st_size;
	info->fd = fd;
	info->url = URL;
	info->type = LOCAL;
	info->mp3_data_start = 0;
	info->mp3_data_end = info->size;

	return info;
}

int local_file_close(struct sound_file_info *file)
{
	int ret;

	ret = close(file->fd);
	if (ret < 0)
		perror("close");

	return ret;
}

int local_file_load(struct sound_file_info *file, u8 *buff, size_t size)
{
	int ret;

	lseek(file->fd, file->offset, SEEK_SET);

	ret = read(file->fd, buff, size);
	if (ret < 0) {
		perror("read");
		return ret;
	}

	return ret;
}
