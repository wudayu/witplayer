#include <errno.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include "witplay.h"

#define LOAD_BUFF (200 * 1024)
#define MP3_BUFF_SIZE 4096
#define RAW_BUFF_SIZE 4608

struct load_thread_arg {
	struct sound_file_info *file;
	struct fifo *fifo;
};

static void *load_mp3_data_to_fifo(void *arg)
{
	struct sound_file_info *file;
	struct fifo *fifo;
	u8 buff[LOAD_BUFF];
	size_t size;

	file = ((struct load_thread_arg *)arg)->file;
	fifo = ((struct load_thread_arg *)arg)->fifo;

	file->offset = file->mp3_data_start;

	while (file->offset < file->mp3_data_end) {
		size = file->mp3_data_end - file->offset;
		size = size > LOAD_BUFF ? LOAD_BUFF : size;
		size = sound_file_load(file, buff, LOAD_BUFF);
		if (size < 0)
			break;

		fifo_write(fifo, buff, size);
	}

	return NULL;
}

int main(int argc, char *argv[])
{
	int ret;
	const char *url;
	struct sound_file_info *file;
	struct decode *dec;
	struct fifo *fifo;
	pthread_t tid;
	struct load_thread_arg arg;
	u8 *lrc;
	u8 *icon;
	size_t lrc_size;
	size_t icon_size;
	u8 mp3_buff[MP3_BUFF_SIZE];
	u8 raw_buff[RAW_BUFF_SIZE];
	int mp3_size, raw_size;
	struct mp3_param mp3_pm;
	struct audio_output *out;
	struct window_info *win_info;

	if (argc < 2) {
		fprintf(stderr, "Usage: %s PATH\n", argv[0]);
		return -EINVAL;
	}

	url = argv[1];

	file = sound_file_open(url);
	if (NULL == file) {
		fprintf(stderr, "Fail to open sound file \"%s\"!\n", url);
		return -ENODEV;
	}

	fifo = fifo_open();
	if (NULL == fifo) {
		goto L1;
		ret = -ENOMEM;
	}

	ret = parse_mp3_tag(file, &lrc, &lrc_size, &icon, &icon_size);
	if (ret < 0) {
		DPRINT("\n");
		goto L2;
	}

	DPRINT("mp3_start = %lu, mp3_end = %lu, "
			"lrc = %p, lrc_size = %lu, icon = %p, icon_size = %lu\n",
			file->mp3_data_start, file->mp3_data_end,
			lrc, lrc_size, icon, icon_size);

	arg.fifo = fifo;
	arg.file = file;
	ret = pthread_create(&tid, NULL, load_mp3_data_to_fifo, &arg);
	if (ret < 0) {
		DPRINT("\n");
		goto L2;
	}

	dec = decode_open(MPAUDEC); // fixme!
	if (NULL == dec) {
		ret = -ENODEV;
		goto L2;
	}

	while (fifo->used < fifo->size / 3) usleep(1000);
	mp3_size = fifo_read(fifo, mp3_buff, sizeof(mp3_buff));

	get_mp3_param(dec, mp3_buff, mp3_size, &mp3_pm);

	win_info = window_init();
	win_info->icon = icon;
	win_info->icon_size = icon_size;
	win_info->lrc = lrc;
	win_info->lrc_size = lrc_size;
	win_info->total.tv_sec = (file->mp3_data_end - file->mp3_data_start) * 8 / mp3_pm.bit_rate;
	win_info->total.tv_usec = (file->mp3_data_end - file->mp3_data_start) * 8 * 1000000 / mp3_pm.bit_rate % 1000000;
	win_info->param = &mp3_pm;

	DPRINT("rate = %d, channels = %d, bps = %d, bitrate = %d\n",
			mp3_pm.rate, mp3_pm.channels, mp3_pm.bits_per_sample, mp3_pm.bit_rate);

	out = open_audio(AUDIO_ALSA, &mp3_pm);
	if (NULL == out) {
		ret = -ENODEV;
		goto L3;
	}

	while (1) {
		if (file->mp3_data_end == file->offset && mp3_size == 0)
			break;

		if (mp3_size > 0) {
			ret = decode(dec, raw_buff, &raw_size, mp3_buff, mp3_size);
			mp3_size -= ret;
			memmove(mp3_buff, mp3_buff + ret, mp3_size);
		}

		play_frames(out, raw_buff, raw_size, &mp3_pm);

		ret = fifo_read(fifo, mp3_buff + mp3_size, sizeof(mp3_buff) - mp3_size);

		mp3_size += ret;
	}

	close_audio(out);
	window_destroy();
L3:
	decode_close(dec);
L2:
	fifo_close(fifo);
L1:
	sound_file_close(file);

	return ret;
}
