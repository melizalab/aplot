
/* Copyright (C) 2004 by Amish S. Dave (adave@ucla.edu) */

/*
** INCLUDE FILES
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#define ALSA_PCM_NEW_HW_PARAMS_API
#include <alsa/asoundlib.h>


static char *pdevice = "hw:0,0";
static snd_pcm_t *writehandle = NULL;
static int writeframesize = 0;

static int setparams(snd_pcm_t *phandle, int ndacchans, int *dacrate_p);


/*************************************************************
*************************************************************/
int init_alsa(int dacrate, int ndacchans)
{
	int retval;
	int err;

	writehandle = NULL;
	retval = -1;

	if ((err = snd_pcm_open(&writehandle, pdevice, SND_PCM_STREAM_PLAYBACK, 0)) < 0)
	{
		fprintf(stderr, "alsa: cannot open audio device %s for output: (%s)\n", pdevice, snd_strerror(err));
		goto finished;
	}

	if (setparams(writehandle, ndacchans, &dacrate) < 0)
		goto finished;

	writeframesize = sizeof(short) * ndacchans;

	retval = 0;

finished:
	if (retval == -1)
	{
		if (writehandle)
			snd_pcm_close(writehandle);
		writehandle = NULL;
		writeframesize = 0;
	}
	return retval;
}


/*************************************************************
*************************************************************/
void close_alsa(void)
{
	if (writehandle)
	{
		snd_pcm_close(writehandle);
	}
	writehandle = NULL;
	writeframesize = 0;
	return;
}


/*************************************************************
*************************************************************/
int write_alsa(short *databuf, int cnt)
{
	int nframes, rc;

	nframes = snd_pcm_writei(writehandle, databuf, cnt / writeframesize);
	if ((nframes < 0) && (errno == EWOULDBLOCK))
	{
		rc = 0;
		goto writefinished;
	}
	if (nframes < 0)
	{
		fprintf(stdout, "alsa: write returned error %d: %s\n", nframes, snd_strerror(nframes));
		rc = -1;
	}
	else rc = nframes * writeframesize;
writefinished:
	return rc;
}


/*************************************************************
*************************************************************/
int trig_alsa(void)
{
	int err;

	err = snd_pcm_start(writehandle);
	if (err < 0)
	{
		fprintf(stderr, "alsa: snd_pcm_start returned error: %s\n", snd_strerror(err));
		return -1;
	}
	return 0;
}


/*************************************************************
*************************************************************/
static int setparams(snd_pcm_t *phandle, int ndacchans, int *dacrate_p)
{
	int err;
	snd_pcm_hw_params_t *pt_params;    /* templates with rate, format and channels */
	snd_pcm_hw_params_t *p_params;
	snd_pcm_sw_params_t *p_swparams;
	snd_pcm_uframes_t p_size, p_psize;
	unsigned int rrate, samplerate;
	snd_pcm_uframes_t val;
	static int latency_min = 4096;
	static int latency_max = 20480;
	int bufsize, last_bufsize;

	snd_pcm_hw_params_alloca(&p_params);
	snd_pcm_hw_params_alloca(&pt_params);
	snd_pcm_sw_params_alloca(&p_swparams);

	err = snd_pcm_hw_params_any(phandle, pt_params);
	if (err < 0) {
		fprintf(stderr, "alsa: Broken configuration for PCM playback: no configurations available: %s\n", snd_strerror(err));
		return err;
	}
	err = snd_pcm_hw_params_set_access(phandle, pt_params, SND_PCM_ACCESS_RW_INTERLEAVED);
	if (err < 0) {
		fprintf(stderr, "alsa: Access type not available for playback: %s\n", snd_strerror(err));
		return err;
	}
	err = snd_pcm_hw_params_set_format(phandle, pt_params, SND_PCM_FORMAT_S16_LE);
	if (err < 0) {
		fprintf(stderr, "alsa: Sample format not available for playback: %s\n", snd_strerror(err));
		return err;
	}
	err = snd_pcm_hw_params_set_channels(phandle, pt_params, ndacchans);
	if (err < 0) {
		fprintf(stderr, "alsa: Channels count (%i) not available for playback: %s\n", ndacchans, snd_strerror(err));
		return err;
	}
	samplerate = *dacrate_p;
	rrate = samplerate;
	err = snd_pcm_hw_params_set_rate_near(phandle, pt_params, &rrate, 0);
	if (err < 0) {
		fprintf(stderr, "alsa: Rate %iHz not available for playback: %s\n", samplerate, snd_strerror(err));
		return err;
	}
	if (rrate != samplerate) {
		fprintf(stderr, "alsa: Rate doesn't match (requested %iHz, get %iHz)\n", samplerate, rrate);
		*dacrate_p = rrate;
	}

	bufsize = latency_min;
	last_bufsize = 0;
__again:
	if (last_bufsize == bufsize)
		bufsize += 4;
	last_bufsize = bufsize;
	if (bufsize > latency_max)
		return -1;

	snd_pcm_hw_params_copy(p_params, pt_params);
	p_psize = bufsize * 2;
	err = snd_pcm_hw_params_set_buffer_size_near(phandle, p_params, &p_psize);
	if (err < 0) {
		fprintf(stderr, "alsa: Unable to set buffer size %d for playback: %s\n", bufsize * 2, snd_strerror(err));
		exit(-1);
	}
	p_psize /= 2;
	err = snd_pcm_hw_params_set_period_size_near(phandle, p_params, &p_psize, 0);
	if (err < 0) {
		fprintf(stderr, "alsa: Unable to set period size %li for playback: %s\n", p_psize, snd_strerror(err));
		exit(-1);
	}

	snd_pcm_hw_params_get_period_size(p_params, &p_psize, NULL);
	snd_pcm_hw_params_get_buffer_size(p_params, &p_size);
	if (p_psize > bufsize)
		bufsize = p_psize;
	if (p_psize * 2 < p_size)
		goto __again;

	err = snd_pcm_hw_params(phandle, p_params);
	if (err < 0) {
		fprintf(stderr, "alsa: Unable to set hw params for playback: %s\n", snd_strerror(err));
		return err;
	}
	err = snd_pcm_sw_params_current(phandle, p_swparams);
	if (err < 0) {
		fprintf(stderr, "alsa: Unable to determine current swparams for playback: %s\n", snd_strerror(err));
		return err;
	}
//	err = snd_pcm_sw_params_set_start_threshold(phandle, p_swparams, 0x7fffffff);
	err = snd_pcm_sw_params_set_start_threshold(phandle, p_swparams, 0x00000fff);
	if (err < 0) {
		fprintf(stderr, "alsa: Unable to set start threshold mode for playback: %s\n", snd_strerror(err));
		return err;
	}
	err = snd_pcm_sw_params_set_stop_threshold(phandle, p_swparams, 0x7fffffff);
	if (err < 0) {
		fprintf(stdout, "alsa: Unable to set stop threshold mode for playback: %s\n", snd_strerror(err));
		return err;
	}
	snd_pcm_hw_params_get_period_size(p_params, &val, NULL);
	err = snd_pcm_sw_params_set_avail_min(phandle, p_swparams, val);
	if (err < 0) {
		fprintf(stderr, "alsa: Unable to set avail min for playback: %s\n", snd_strerror(err));
		return err;
	}
	val = 1;
	err = snd_pcm_sw_params_set_xfer_align(phandle, p_swparams, val);
	if (err < 0) {
		fprintf(stderr, "alsa: Unable to set transfer align for playback: %s\n", snd_strerror(err));
		return err;
	}
	err = snd_pcm_sw_params(phandle, p_swparams);
	if (err < 0) {
		fprintf(stderr, "alsa: Unable to set sw params for playback: %s\n", snd_strerror(err));
		return err;
	}

	if ((err = snd_pcm_prepare(phandle)) < 0) {
		fprintf(stderr, "alsa: Prepare error: %s\n", snd_strerror(err));
		exit(0);
	}
	return 0;
}

