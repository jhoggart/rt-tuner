/**
* @file		AudioCapturer.h
* @author	Justin Hoggart <jwhoggart@gmail.com>
* @version	1.0
*
* The AudioCapturer class is responsible for interfacing with the
* microphone and obtaining the raw audio data.
*/

#pragma once

// Standard includes:
#include <iostream>
#include <cstdlib>
#include <cstring>

// Local includes:
#include "AudioProcessor.h"
#include "RTAudio\RTAudio.h"
#include "wxmathplot\mathplot.h"

// Constants:
#define AUDIO_NUM_CHANNELS 1			// number of audio channels to use (default: 1)
#define AUDIO_SAMPLE_RATE 8000			// audio sample rate (default: 8000)
#define AUDIO_BUFFER_FRAMES 4096		// number of sample frames (default: 4096)
#define AUDIO_DOWNSAMPLE_FACTOR 4		// number of times to downsample, used for HPS algorithm (default: 4)
#define MAX_FREQ 330					// the highest frequency to consider (default: C4, or ~262 Hz)
#define MIN_FREQ 29						// the lowest frequency to consider (default: B0, or ~30 Hz)

// Forward declarations:
class AudioProcessor;

// Structure giving the RtAudio callback function access to the features that it needs
struct userdata
{
	AudioProcessor*		proc;
	mpFXYVector*		vector;
	double*				totalfreq;
	int*				freqcount;

	userdata() {
		proc = NULL;
		vector = NULL;
		totalfreq = NULL;
		freqcount = NULL;
	}

	userdata(AudioProcessor* p, mpFXYVector* v, double* f, int* c) {
		proc = p;
		vector = v;
		totalfreq = f;
		freqcount = c;
	}
};


class AudioCapturer
{
public:
	// Constructors/destructors:
	AudioCapturer(mpFXYVector* vec, double* totfreq, int* freqcount);
	~AudioCapturer();

	// Instance variables:
	userdata*	udata;

	// Methods:
	static int	CaptureAudio(void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
							double streamTime, RtAudioStreamStatus status, void *userData);
	int			InitializeAudio();
	int			StartCapture();
	int			StopCapture();
	int			CloseStream();

private:
	// Private variables:
	AudioProcessor*		processor;
	RtAudio*			device;
};