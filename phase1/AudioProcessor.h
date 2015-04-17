/**
* @file		AudioCapturer.h
* @author	Justin Hoggart <jwhoggart@gmail.com>
* @version	1.0
*
* The AudioCapturer class is responsible for interfacing with the
* microphone and obtaining the raw audio data.
*/

#pragma once
#define _USE_MATH_DEFINES	// Needed for M_PI

// Standard includes:
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <complex>
#include <cstring>
#include <algorithm>
#include <math.h>

// Local includes:
#include "FFTW\fftw3.h"

class AudioProcessor
{
public:
	// Methods:
	std::vector<double>	PerformFFT(double* data, int fftsize);
	int					HPS(std::vector<double> spectrum, int harmonics);
	static void			ApplyWindowFunction(double* data, int size);
	static void			CalcLowPassParams(double samplerate, double maxfrequency, double *a, double *b);
	static double		LowPass(double x, double* mem, double* a, double* b);
};