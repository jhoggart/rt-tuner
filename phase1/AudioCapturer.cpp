/**
* @file		AudioCapturer.cpp
* @author	Justin Hoggart <jwhoggart@gmail.com>
* @version	1.0
*
* The AudioCapturer class is responsible for interfacing with the 
* microphone.
**/

#include "AudioCapturer.h"

// ****** Constructors:
AudioCapturer::AudioCapturer(mpFXYVector* vector, double* totalfreq, int* freqcount) 
{
	// initialize instance variables
	try {
		device = new RtAudio();
	}
	catch (RtAudioError &err) {
		err.printMessage();
		exit(EXIT_FAILURE);
	}

	processor = new AudioProcessor();
	udata = new userdata(processor, vector, totalfreq, freqcount);
}

// ****** Destructor:
AudioCapturer::~AudioCapturer() 
{
	// clean up the pre-allocated instance objects
	delete this->udata;
	delete this->processor;
	delete this->device;
}

// ****** Methods:
// Callback method for use with RtAudio
int AudioCapturer::CaptureAudio(void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
								double streamTime, RtAudioStreamStatus status, void *userData)
{
	// interpret the userData in the context of our structure
	userdata* udata = (userdata*)userData;

	if (inputBuffer != NULL) {
		double* data = (double*)inputBuffer;

		// use low-pass filter to limit noise from high frequencies
		double a[2], b[3], mem1[4], mem2[4];
		for (int i = 0; i < 4; i += 1) {
			mem1[i] = 0;
			mem2[i] = 0;
		}
		udata->proc->CalcLowPassParams(AUDIO_SAMPLE_RATE, MAX_FREQ, a, b);
		for (size_t i = 0; i < nBufferFrames; i += 1) {
			// apply filter twice for good measure
			data[i] = udata->proc->LowPass(data[i], mem1, a, b);
			data[i] = udata->proc->LowPass(data[i], mem2, a, b);
		}

		// apply the windowing function to clean up the edges of the audio sample
		udata->proc->ApplyWindowFunction(data, nBufferFrames);
		// calculate the Fast Fourier Transform for the audio data
		std::vector<double> temp = udata->proc->PerformFFT(data, nBufferFrames);

		// store the frequency data in a graph-friendly format
		std::vector<double> xs, spectrum, logspectrum;
		for (size_t i = 0; i < (temp.size() / 2); i += 1) {
			// for plotting on a logarithmic scale
			logspectrum.push_back(10 * log10(temp.at(i)));
			// for plotting on a linear scale
			spectrum.push_back(temp.at(i));
			xs.push_back(i);
		}

		// Update the frequency spectrum graph
		udata->vector->SetData(xs, logspectrum);
		//udata->vector->SetData(xs, spectrum);

		// Perform the Harmonic Product Spectrum and determine the fundamental frequency
		int fundamentalBin = udata->proc->HPS(spectrum, AUDIO_DOWNSAMPLE_FACTOR);
		double binSize = (double)AUDIO_SAMPLE_RATE / (double)AUDIO_BUFFER_FRAMES;
		double fundamental = ((double)fundamentalBin * binSize);
		
		// accumulate frequency data over time, so we can average the results
		*(udata->totalfreq) += fundamental;
		*(udata->freqcount) += 1;
	}

	return 0;
}

// Start the audio capture stream
int AudioCapturer::InitializeAudio()
{
	// ensure that the computer has an available audio device
	if (this->device->getDeviceCount() < 1) {
		std::cout << "No audio devices found!\n";
		//this->WriteToGraphLog("No audio devices found!");
		return -1;
	}

	// initialize the audio device input parameters
	unsigned int bufferFrames = AUDIO_BUFFER_FRAMES, sampleRate = AUDIO_SAMPLE_RATE;
	RtAudio::StreamParameters iParams, oParams;
	iParams.deviceId = this->device->getDefaultInputDevice();
	iParams.nChannels = AUDIO_NUM_CHANNELS;
	iParams.firstChannel = 0;
	
	// open the stream
	try {
		this->device->openStream(NULL, &iParams, RTAUDIO_FLOAT64, sampleRate, &bufferFrames, &AudioCapturer::CaptureAudio, (void*)udata);
	}
	catch (RtAudioError& e) {
		e.printMessage();
		std::cout << "***Problem opening audio stream.\n";
		return -1;
	}

	// begin capturing audio
	this->StartCapture();

	return 0;
}

// Begin the audio capturing process
int AudioCapturer::StartCapture() 
{
	// start the audio stream
	try {
		this->device->startStream();
		return 0;
	}
	catch (RtAudioError& e) {
		e.printMessage();
		return -1;
	}
}

// End the audio capturing process
int AudioCapturer::StopCapture() 
{
	// stop the audio stream
	try {
		this->device->stopStream();
		return 0;
	}
	catch (RtAudioError& e) {
		e.printMessage();

		this->CloseStream();
		return -1;
	}
}

// Close the open audio stream
int AudioCapturer::CloseStream() 
{
	// close the stream
	if (this->device->isStreamOpen()) {
		try {
			this->device->closeStream();
		}
		catch (RtAudioError& e) {
			e.printMessage();
			this->device->abortStream();
			return -1;
		}
	}

	return 0;
}