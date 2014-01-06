#include <alsa/asoundlib.h>
#include "audio_output.h"

#define DEVICE "default"

static int xrun_recovery(snd_pcm_t *handle, int err)
{
	if (err == -EPIPE) { /* under-run */
		err = snd_pcm_prepare(handle);
		if (err < 0)
			printf("Can't recovery from underrun, prepare failed: %s\n",
					snd_strerror(err));
		return 0;
	} else if (err == -ESTRPIPE) {
		while ((err = snd_pcm_resume(handle)) == -EAGAIN)
			sleep(1); /* wait until the suspend flag is released */
		if (err < 0) {
			err = snd_pcm_prepare(handle);
			if (err < 0)
				printf("Can't recovery from suspend, prepare failed: %s\n",
						snd_strerror(err));
		}
		return 0;
	}

	return err;
}

void *open_alsa(struct mp3_param *param)
{
	int ret;
	snd_pcm_t *pcm;
	snd_pcm_hw_params_t *hwparams;

	ret = snd_pcm_open(&pcm, DEVICE, SND_PCM_STREAM_PLAYBACK, 0);
	if (ret < 0) {
		perror("snd_pcm_open");
		return NULL;
	}

	snd_pcm_hw_params_alloca(&hwparams);

	ret = snd_pcm_hw_params_any(pcm, hwparams);
	if (ret < 0) {
		fprintf(stderr, "%s\n", snd_strerror(ret));
		goto L1;
	}

	ret = snd_pcm_hw_params_set_access(pcm, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED);
	if (ret < 0) {
		fprintf(stderr, "%s\n", snd_strerror(ret));
		goto L1;
	}

	ret = snd_pcm_hw_params_set_format(pcm, hwparams, SND_PCM_FORMAT_S16_LE);
	if (ret < 0) {
		fprintf(stderr, "%s\n", snd_strerror(ret));
		goto L1;
	}

	ret = snd_pcm_hw_params_set_channels(pcm, hwparams, param->channels);
	if (ret < 0) {
		fprintf(stderr, "%s\n", snd_strerror(ret));
		goto L1;
	}

	ret = snd_pcm_hw_params_set_rate(pcm, hwparams, param->rate, 0);
	if (ret < 0) {
		fprintf(stderr, "%s\n", snd_strerror(ret));
		goto L1;
	}

	ret = snd_pcm_hw_params(pcm, hwparams);
	if (ret < 0) {
		fprintf(stderr, "%s\n", snd_strerror(ret));
		goto L1;
	}

	ret = snd_pcm_prepare(pcm);
	if (ret < 0) {
		fprintf(stderr, "%s\n", snd_strerror(ret));
		goto L1;
	}

	return pcm;

L1:
	snd_pcm_close(pcm);

	return NULL;
}

int close_alsa(snd_pcm_t *pcm)
{
	snd_pcm_close(pcm);

	return 0;
}

int play_alsa_frames(snd_pcm_t *pcm, u8 *raw_buff, int frames)
{
	int ret;

	ret = snd_pcm_writei(pcm, raw_buff, frames);
	if (ret < 0) {
		ret = xrun_recovery(pcm, ret);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}
