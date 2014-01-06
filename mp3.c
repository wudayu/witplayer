#include "mp3.h"
#include "sound_file.h"
#include "types.h"
#include "mpaudec.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#define TAGV1_SIZE 128
#define ID3_TAG_HEAD_SIZE 10

#define PATH "/maxwit/project/witplayer"
#define TAGV1_CHK(tag) (('T' == (tag)[0]) && ('A' == (tag)[1]) && ('G' == (tag)[2]))

#define TITLE_LEN 512
#define ID3_HEAD_SIZE 10
#define ID3SIZE(ID3_head) (((ID3_head)[6] << 21) | ((ID3_head)[7] << 14) | ((ID3_head)[8] << 7) | ((ID3_head)[9] << 0))

#define FRAME_HEAD_SIZE 10
#define FRAMESIZE_V3(frame) (((frame)[4] << 24) | ((frame)[5] << 16) | ((frame)[6] << 8) | ((frame)[7]))
#define CHECK_TIT2_V3(frame) (((frame)[0] == 'T') && ((frame)[1] == 'I') && ((frame)[2] == 'T') && ((frame)[3] == '2'))
#define CHECK_USLT_V3(frame) (((frame)[0] == 'U') && ((frame)[1] == 'S') && ((frame)[2] == 'L') && ((frame)[3] == 'T'))
#define CHECK_APIC_V3(frame) (((frame)[0] == 'A') && ((frame)[1] == 'P') && ((frame)[2] == 'I') && ((frame)[3] == 'C'))

#define FRAME_HEAD_SIZE_2 6
#define FRAMESIZE_V2(frame) (((frame)[3] << 16) | ((frame)[4] << 8) | ((frame)[5] << 0))
#define CHECK_TT2_V2(frame) (((frame)[0] == 'T') && ((frame)[1] == 'T') && ((frame)[2] == '2'))
#define CHECK_SLT_V2(frame) (((frame)[0] == 'S') && ((frame)[1] == 'L') && ((frame)[2] == 'T'))
#define CHECK_PIC_V2(frame) (((frame)[0] == 'P') && ((frame)[1] == 'I') && ((frame)[2] == 'C'))

static int parse_ID3v1_tag(u8 *buff, struct sound_file_info *ifno, u8 **lrc, size_t *lrc_size, u8 **icon,size_t *icon_size)
{
	if (TAGV1_CHK(buff)) {
		ifno->mp3_data_end = ifno->size - TAGV1_SIZE;
	} else {
		ifno->mp3_data_end = ifno->size;
	}

	return 0;
}

static int traverse_dir_match(char path[], u8 *title, char *f_path)
{
	DIR *dir;
	char tmp[256];
	size_t ret;
	struct dirent *dir_rent;

	dir = opendir(path);

	while (NULL != (dir_rent = readdir(dir))) {
		if (strcmp(dir_rent->d_name, ".") == 0)
			continue;
		else if (strcmp(dir_rent->d_name, "..") == 0)
			continue;

		strcpy(tmp, path);
		strcat(strcat(tmp, "/"), dir_rent->d_name);

		if (dir_rent->d_type == DT_DIR) {
			ret = traverse_dir_match(tmp, title, f_path);
			if (ret == 0)
				return ret;
		} else {
			if (!strcmp(dir_rent->d_name, (const char *)title)) {
				printf("find it!!\n");
				strcpy(f_path, tmp);
				return 0;
			}
		}
	}

	return -1;
}

static int add_lrc(u8 *title, u8 **lrc, size_t *size)
{
	int fd;
	int ret;
	char file[256];
	struct stat sb;

	ret = traverse_dir_match(PATH, title, file);
	if (ret < 0) {
		printf("not find %s\n", title);
		return ret;
	}

	fd = open(file, O_RDONLY);
	if (fd < 0) {
		perror("open");
		return -1;
	}

	ret = fstat(fd, &sb);
	if (ret < 0) {
		perror("fstat");
		return -1;
	}

	*lrc = malloc(sizeof(u8) * sb.st_size);
	ret = read(fd, *lrc, sb.st_size);
	if (ret < 0) {
		perror("read");
		return -1;
	}

	close(fd);

	*size = ret;

	return 0;
}

static int change_encode(u8 *buff, size_t size)
{
	int i;
	int len = size / 2 - 1;

	if (buff[0] == 0xff && buff[1] == 0xfe) {
		for (i = 0; i < len; i++)
			buff[i] = buff[(i + 1) * 2];

	} else if (buff[0] == 0xfe && buff[1] == 0xff) {
		for (i = 0; i < len; i++)
			buff[i] = buff[(i + 1) * 2 + 1];

	} else
		return -1;

	buff[len] = '\0';

	return len;
}

static int parse_ID3v2_3_tag(u8 *buff, size_t size, u8 **lrc, size_t *lrc_size, u8 **icon, size_t *icon_size)
{
	u8 *p = buff + ID3_HEAD_SIZE;
	u8 title[TITLE_LEN] = {'\0'};
	size_t ID3_size, frame_size;
	int getlrc = 1;
	int geticon = 1;

	*lrc = NULL;
	*icon = NULL;
	*lrc_size = 0;
	*icon_size = 0;

	ID3_size = ID3SIZE(buff);

	while ((getlrc || geticon) && (p < buff + ID3_size + ID3_HEAD_SIZE)) {

		frame_size = FRAMESIZE_V3(p);

		if (frame_size == 0)
			break;

		if (CHECK_USLT_V3(p)) {
			*lrc = (u8 *)malloc(frame_size);
			if (NULL == *lrc) {
				perror("malloc");
				return -1;
			}
			*lrc_size = frame_size - 1;

			memcpy(*lrc, p + FRAME_HEAD_SIZE + 1, *lrc_size);
			if (p[10] == 1 || p[10] == 2)
				*lrc_size = change_encode(*lrc, frame_size - 1);

			getlrc--;

		} else if (CHECK_APIC_V3(p)) {
			*icon = (u8 *)malloc(frame_size);
			if (NULL == *icon) {
				perror("malloc");
				return -1;
			}
			*icon_size = frame_size - 1;

			memcpy(*icon, p + FRAME_HEAD_SIZE + 1, *icon_size);
			geticon--;

		} else if (CHECK_TIT2_V3(p)) {
			memcpy(title, p + FRAME_HEAD_SIZE + 1, frame_size - 1);
			if (p[10] == 1 || p[10] == 2)
				change_encode(title, frame_size - 1);

			title[frame_size - 1] = '\0';
		}

		p += (frame_size + FRAME_HEAD_SIZE);
	}

	if (getlrc && title[0] != '\0')
		add_lrc(title, lrc, lrc_size);

	return 0;
}

static int parse_ID3v2_2_tag(u8 *buff, size_t size, u8 **lrc, size_t *lrc_size, u8 **icon, size_t *icon_size)
{
	u8 *p = buff + ID3_HEAD_SIZE;
	u8 title[TITLE_LEN];
	size_t ID3_size, frame_size;
	int getlrc = 1;
	int geticon = 1;

	*lrc = NULL;
	*icon = NULL;
	*lrc_size = 0;
	*icon_size = 0;

	ID3_size = ID3SIZE(buff);

	while ((getlrc || geticon) && (p < buff + ID3_size + ID3_HEAD_SIZE)) {

		frame_size = FRAMESIZE_V2(p);
			break;

		if (CHECK_SLT_V2(p)) {
			*lrc = (u8 *)malloc(frame_size);
			if (NULL == *lrc) {
				perror("malloc");
				return -1;
			}
			*lrc_size = frame_size - 1;

			memcpy(*lrc, p + FRAME_HEAD_SIZE_2 + 1, *lrc_size);
			if (p[10] == 1 || p[10] == 2)
				*lrc_size = change_encode(*lrc, frame_size - 1);
			getlrc--;

		} else if (CHECK_PIC_V2(p)) {
			*icon = (u8 *)malloc(frame_size);
			if (NULL == *icon) {
				perror("malloc");
				return -1;
			}
			*icon_size = frame_size - 1;

			memcpy(*icon, p + FRAME_HEAD_SIZE_2 + 1, *icon_size);
			geticon--;

		} else if (CHECK_TT2_V2(p)) {
			memcpy(title, p + FRAME_HEAD_SIZE_2 + 1, frame_size - 1);
			if (p[10] == 1 || p[10] == 2)
				*lrc_size = change_encode(title, frame_size - 1);
			title[frame_size - 1] = '\0';
		}

		p += (frame_size + FRAME_HEAD_SIZE_2);
	}

	if (getlrc && title[0] != '\0')
		add_lrc(title, lrc, lrc_size);

	return 0;
}

static int parse_ID3v2_tag(u8 *buff, size_t size, u8 **lrc, size_t *lrc_size, u8 **icon, size_t *icon_size)
{
	char ver = buff[3];

	if (ver == 2)
		parse_ID3v2_2_tag(buff, size, lrc, lrc_size, icon, icon_size);
	else if (ver == 3)
		parse_ID3v2_3_tag(buff, size, lrc, lrc_size, icon, icon_size);
	else if (ver == 4)
		parse_ID3v2_3_tag(buff, size, lrc, lrc_size, icon, icon_size);
	else
		return -1;

	return 0;
}

int free_pares_mp3_tag(void *lrc, void *icon)
{
	if (NULL == icon)
		free(icon);

	if (NULL == lrc)
		free(lrc);

	return 0;
}

int parse_mp3_tag(struct sound_file_info *ifno, u8 **lrc, size_t *lrc_size, u8 **icon, size_t *icon_size)
{
	int ret;
	u8 head[ID3_TAG_HEAD_SIZE];

	sound_file_seek(ifno, 0);
	sound_file_load(ifno, head, sizeof(head));
	if (head[0] == 'I' && head[1] == 'D' && head[2] == '3') {
		u8 *buff_v2;
		u8 buff_v1[128];
		size_t ID3_size;

		ID3_size = ID3SIZE(head);
		ifno->mp3_data_start = ID3_size + ID3_HEAD_SIZE;

		buff_v2 = malloc(ID3_HEAD_SIZE + ID3_size);
		if (NULL == buff_v2) {
			DPRINT("\n");
			return -1;
		}

		sound_file_seek(ifno, 0);
		sound_file_load(ifno, buff_v2, ID3_HEAD_SIZE + ID3SIZE(head));

		ret = parse_ID3v2_tag(buff_v2, ID3_HEAD_SIZE + ID3SIZE(head), lrc, lrc_size, icon, icon_size);
		if (ret < 0)
			fprintf(stderr,"parse_ID3v2_tag()");

		sound_file_seek(ifno, ifno->size - 128);
		sound_file_load(ifno, buff_v1, 128);

		ret = parse_ID3v1_tag(buff_v1, ifno, lrc, lrc_size, icon, icon_size);
		if (ret < 0)
			fprintf(stderr,"parse_ID3v1_tag()");

		free(buff_v2);
	}

	return 0;
}

int get_mp3_param(struct decode *dec, u8 *buff, size_t size, struct mp3_param *param)
{
	if (dec->type == MPAUDEC) {
		char raw_buff[MPAUDEC_MAX_AUDIO_FRAME_SIZE];
		int raw_size;

		MPAuDecContext *mdec;

		mdec = dec->dec;

		mdec->parse_only = 1;
		mpaudec_decode_frame(mdec, raw_buff, &raw_size, buff, size);
		param->bits_per_sample = 16;
		param->channels = mdec->channels;
		param->rate = mdec->sample_rate;
		param->bit_rate = mdec->bit_rate;

		mdec->parse_only = 0;

		return 0;
	}

	return -1;
}

struct decode *decode_open(decode_type_t type)
{
	struct decode *dec;

	dec = malloc(sizeof(*dec));
	if (dec == NULL) {
		perror("malloc");
		return NULL;
	}

	dec->type = type;

	switch (type) {
	case MPAUDEC:
		dec->dec = mpaudec_open();
		break;
	case GSTREAMERDEC:
		dec->dec = NULL;
		break;
	}

	return dec;
}

int decode_close(struct decode *dec)
{
	switch (dec->type) {
	case MPAUDEC:
		// mpaudec_close(dec->dec);
		break;
	case GSTREAMERDEC:
		break;
	}

	free(dec);

	return 0;
}

int decode(struct decode *dec, u8 *raw_buff, int *raw_size, u8 *mp3_buff, size_t mp3_size)
{
	if (dec->type == MPAUDEC) {
		return mpaudec_decode_frame(dec->dec, raw_buff, raw_size, mp3_buff, mp3_size);
	}

	return 0;
}
