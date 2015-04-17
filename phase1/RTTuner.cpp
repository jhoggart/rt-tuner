/**
* @file		RTTuner.cpp
* @author	Justin Hoggart <jwhoggart@gmail.com>
* @version	1.0
*
* RTTuner is the driver class for the RT-Tuner application
*/

#pragma once
//#define _USE_MATH_DEFINES	// Needed for M_PI

// Local includes:
#include "AudioVisualizer.h"

// wxWidgets includes:
#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#endif
#include <wx/wx.h>

// Class definition:
class RTTuner : public wxApp
{
public:
	virtual bool OnInit();
};

// Launch main application:
IMPLEMENT_APP(RTTuner)


// Class implementation:
bool RTTuner::OnInit() 
{
	// launch the GUI window
	wxInitAllImageHandlers();
	wxFrame *frame = new AudioVisualizer();
	frame->Show(TRUE);
	((AudioVisualizer*)frame)->WriteToGraphLog("Starting Audio functions...");

	// start the audio functions
	if (((AudioVisualizer*)frame)->InitializeAudio() == -1) {
		std::cout << "Error encountered. Exiting...\n";
		exit(0);
	}
	((AudioVisualizer*)frame)->WriteToGraphLog("Audio stream started.");

	return TRUE;
}