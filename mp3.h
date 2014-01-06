#pragma once
#include <stdio.h>
#include "sound_file.h"
#include "fifo.h"
#include "types.h"

typedef enum {
	MPAUDEC,
	GSTREAMERDEC
} decode_type_t;

struct mp3_param {
	int rate;
	int channels;
	int bits_per_sample;
	int bit_rate;
};

struct decode {
	decode_type_t type;
	void *dec;
};

int parse_mp3_tag(struct sound_file_info *, u8 **lrc, size_t *lrc_size, u8 **icon, size_t *icon_size);
int get_mp3_param(struct decode *dec, u8 *buff, size_t size, struct mp3_param *param);
struct decode *decode_open(decode_type_t type);
int decode_close(struct decode *dec);
int decode(struct decode *dec, u8 *raw_buff, int *raw_size, u8 *mp3_buff, size_t mp3_size);
int free_pares_mp3_tag(void *lrc, void *icon);
