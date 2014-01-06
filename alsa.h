#pragma once

void *open_alsa(struct mp3_param *param);
int close_alsa(struct audio_output *out);
int play_alsa_frames(struct audio_output *out, u8 *raw_buff, int frames);
