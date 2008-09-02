
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
#include <sys/poll.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/soundcard.h>


static int dacfd = -1;


/*************************************************************
*************************************************************/
int init_oss(int dacrate, int ndacchans)
{
	int retval;

	int bits=16, volume, left, right;
	int arg;
	audio_buf_info info;

	retval = -1;

	/*
	** Setup the sound driver
	** Note that are changing the volume here
	*/
	if ((dacfd = open("/dev/dsp", O_WRONLY)) == -1)
	{
		fprintf(stdout, "oss: open(/dev/dsp) returned error: %s\n", strerror(errno));
		goto finished;
	}
	ioctl(dacfd, SOUND_PCM_WRITE_BITS, &bits);
	ioctl(dacfd, SOUND_PCM_WRITE_CHANNELS, &ndacchans);
	ioctl(dacfd, SNDCTL_DSP_SPEED, &dacrate);
	left = right = 50;
	volume = (left | (right << 8));
	ioctl(dacfd, MIXER_WRITE(13), &volume);

	arg = 0x0002000B;  /* 0xMMMMSSSS; where MMMM is # of fragments, SSSS is log2(size) of each fragment */
	if (ioctl(dacfd, SNDCTL_DSP_SETFRAGMENT, &arg) == -1)
	{
		fprintf(stdout, "oss: ioctl(SNDCTL_DSP_SETFRAGMENT) returned error: %s\n", strerror(errno));
	}
	ioctl(dacfd, SNDCTL_DSP_GETOSPACE, &info);

	retval = 0;

finished:
	if (retval == -1)
	{
		if (dacfd != -1)
		{
			close(dacfd);
			dacfd = -1;
		}
	}
	return retval;
}


/*************************************************************
*************************************************************/
void close_oss(void)
{
	if (dacfd != -1)
	{
		close(dacfd);
		dacfd = -1;
	}
	return;
}


/*************************************************************
*************************************************************/
int pollout_oss(int timeout_ms)
{
	struct pollfd writepollfds[1];
	int writepollfds_count = 1;
	int rc, revents;

	writepollfds[0].fd = dacfd;
	writepollfds[0].events = POLLOUT | POLLERR;
	writepollfds[0].revents = 0;
	do
	{
		rc = poll(writepollfds, writepollfds_count, timeout_ms);
	} while ((rc == -1) && (errno == EINTR));
	revents = writepollfds[0].revents;
	if (revents & POLLOUT)
		rc = 1;
	if (revents & POLLERR)
		rc = -1;
	return rc;
}


/*************************************************************
*************************************************************/
int write_oss(short *databuf, int cnt)
{
	int err;

	do
	{
		err = write(dacfd, databuf, cnt);
	} while ((err < 0) && (errno == EINTR));

	if ((err < 0) && (errno == EWOULDBLOCK))
	{
		err = 0;
		goto writefinished;
	}
	if (err < 0)
	{
		fprintf(stdout, "oss: write returned error: %s\n", strerror(errno));
		err = -1;
	}
writefinished:
	return err;
}


/*************************************************************
*************************************************************/
int trig_oss(void)
{
	return 0;
}


