

#include <portaudio.h>
#include "aplot.h"

PaStream *stream;

int 
init_sound(int dacrate, int ndachans)
{
	PaStreamParameters outputParameters;
	PaError err;


	/* -- initialize PortAudio -- */
	err = Pa_Initialize();
	if( err != paNoError ) goto error;
	/* -- check for valid devices -- */
	int numDevices = Pa_GetDeviceCount();
	if (numDevices < 1) {
		Pa_Terminate();
		fprintf(stderr, "No sound output devices!\n");
		return -1;
	}

	/* -- setup  output -- */
	outputParameters.device = Pa_GetDefaultOutputDevice(); /* default output device */
	outputParameters.channelCount = ndachans;
	outputParameters.sampleFormat = paInt16;
	outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultHighOutputLatency;
	outputParameters.hostApiSpecificStreamInfo = NULL;

	/* -- setup stream -- */
	err = Pa_OpenStream(
		&stream,
		NULL,
		&outputParameters,
		dacrate,
		SND_FRAMES_PER_BUFFER,
		paClipOff,      /* we won't output out of range samples so don't bother clipping them */
		NULL, /* no callback, use blocking API */
		NULL ); /* no callback, so no callback userData */
	if( err != paNoError ) goto error;

	/* -- start stream -- */
	err = Pa_StartStream( stream );
	if (err != paNoError) goto error;

	return 0;
error:
	Pa_Terminate();
	fprintf(stderr, "Error initializing sound output: %s\n", Pa_GetErrorText(err));
	return -1;
}

void 
close_sound(void) {

	if (stream) {
		Pa_CloseStream( stream );
	}
	Pa_Terminate();
}

int
write_sound(short *databuf, int cnt)
{
	PaError err = Pa_WriteStream(stream, databuf, cnt);
	if ( err != paNoError) {
		fprintf(stderr, "Error outputting sound: %s\n", Pa_GetErrorText(err));
		return -1;
	}
	return 0;
}

