/**
* @file		AudioProcessor.cpp
* @author	Justin Hoggart <jwhoggart@gmail.com>
* @version	1.0
*
* The AudioProcessor class is responsible for processing the audio signal.
* Contains methods for performing the filtering, windowing, and FFT.
**/

#include "AudioProcessor.h"

// Fast Fourier Transform method for converting a raw audio	data to the frequency domain
std::vector<double> AudioProcessor::PerformFFT(double* data, int fftsize)
{
	// allocate space for the output vector
	fftw_complex* out;
	out = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * fftsize);
	
	// calculate the FFTW real-to-complex "plan"
	fftw_plan p;
	p = fftw_plan_dft_r2c_1d(fftsize, data, out, FFTW_ESTIMATE);
	
	// execute the FFT and discard the plan when finished
	fftw_execute(p);
	fftw_destroy_plan(p);
	
	// Calculate the magnitude of the spectrum data
	std::vector<double> spectrum;
	double re, im;
	for (int i = 0; i < fftsize; i += 1) {
		re = out[i][0];
		im = out[i][1];
		spectrum.push_back( sqrt(re*re + im*im) );
	}

	// clean up and return
	fftw_free(out);
	return spectrum;
}

// Harmonic Product Spectrum algorithm for pitch (fundamental frequency) estimation
int AudioProcessor::HPS(std::vector<double> spectrum, int downsampleFactor) 
{
	//*** downsample factor is the number of times to divide the signal before multiplying out

	// limit the search range based on the downsample factor
	int maxI = spectrum.size() / downsampleFactor;
	int bin = 1;
	std::vector<double> hps(spectrum);
	for (int j = 1; j <= maxI; j += 1) {
		// downsample and multiply
		for (int i = 1; i <= downsampleFactor; i += 1) {
			hps.at(j) *= spectrum.at(j*i-1);
		}

		// Determine the highest possible target bin
		if (hps.at(j) > hps.at(bin)) {
			bin = j;
		}
	}

	// attempt to fix octave misidentification errors (harmonics)
	int fixBin = 1;
	int max = bin * 3 / 4;
	
	for (int i = 2; i < max; i += 1) {
		if (hps.at(i) > hps.at(fixBin)) {
			fixBin = i;
		}
	}

	if (abs(fixBin * 2 - bin) < 4) {
		if (hps.at(fixBin) / hps.at(bin) > 0.2) {
			bin = fixBin;
		}
	}

	// return the bin number containing the likely fundamental
	return bin;
}

// Windowing function to clean up the raw audio data before processing it with the FFT
void AudioProcessor::ApplyWindowFunction(double* data, int size)
{	
	for (int i = 0; i < size; i += 1) {
		// use the Hanning window function
		data[i] *= 0.5 * (1 - cos(2 * M_PI * i / (size - 1.0)));
	}
}


// ***NOTE***
// The following two Low-Pass Filter methods were written by Bjorn Roche and are used here with 
// absolutely no claim of credit.
//
// The code for these was found by following the link in his blog post here:
// http://blog.bjornroche.com/2012/07/frequency-detection-using-fft-aka-pitch.html


// Calculate the parameters to be used for the low-pass filter
void AudioProcessor::CalcLowPassParams(double samplerate, double maxfrequency, double *a, double *b)
{
	double w0 = 2 * M_PI * maxfrequency / samplerate;
	double cosw0 = cos(w0), sinw0 = sin(w0);
	double alpha = sinw0 / 2 * sqrt(2);
	
	double a0 = 1 + alpha;
	a[0] = (-2 * cosw0) / a0;
	a[1] = (1 - alpha) / a0;
	
	b[0] = ((1 - cosw0) / 2) / a0;
	b[1] = (1 - cosw0) / a0;
	b[2] = ((1 - cosw0) / 2) / a0;
}

// Low-pass filter to clean up the audio signal, in order to reduce the chances of a "false positive"
// when performing pitch estimation.
double AudioProcessor::LowPass(double x, double* mem, double* a, double* b)
{
	double result = (b[0] * x) + (b[1] * mem[0]) + (b[2] * mem[1])
					- (a[0] * mem[2]) - (a[1] * mem[3]);
	
	mem[1] = mem[0];
	mem[0] = x;
	mem[3] = mem[2];
	mem[2] = result;

	return result;
}