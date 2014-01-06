#pragma once

#include "types.h"
#include "mp3.h"

typedef enum {
	AUDIO_ALSA,
	AUDIO_GSTREAMER
} audio_dev_t;

struct audio_output {
	audio_dev_t type;
	void *outdev;
};

struct audio_output *open_audio(audio_dev_t type, struct mp3_param *param);
int close_audio(struct audio_output *out);
int play_frames(struct audio_output *out, u8 *raw_buff, size_t size, struct mp3_param *param);
