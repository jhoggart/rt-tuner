/**
* @file		AudioVisualizer.h
* @author	Justin Hoggart <jwhoggart@gmail.com>
* @version	1.0
*
* The AudioVisualizer class is responsible for the visualization of the
* audio data.
*/

#pragma once

// Standard includes:
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <vector>

// Local includes:
#include "AudioCapturer.h"
#include "wxmathplot\mathplot.h"
#include "kwic\angularmeter.h"
#include "rtaudio\RTAudio.h"

// wxWidgets includes:
#include <wx\wxprec.h>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx\image.h>
#include <wx\listctrl.h>
#include <wx\sizer.h>
#include <wx\log.h>
#include <wx\intl.h>
#include <wx\print.h>
#include <wx\filename.h>
#include <wx\textctrl.h>
#include <wx\notebook.h>

#define REFRESH_INTERVAL 100

// Forward declarations:
class AudioCapturer;

class AudioVisualizer : public wxFrame 
{
public:
	// Constructors/destructors:
	AudioVisualizer();

	// Methods:
	void	SetUpAngularMeter(wxPanel* panel);
	void	WriteToGraphLog(wxString msg);
	void	WriteNote();
	void	FreqToNote(double freq);
	void	RefreshWindow();
	int		InitializeAudio();
	// -- event handlers
	void	OnQuit(wxCommandEvent &event);
	void	OnClose(wxCloseEvent& event);
	void	OnFit(wxCommandEvent &event);
	void	OnStartStopButton(wxCommandEvent& event);
	void	OnRefreshTimer(wxTimerEvent& event);

	// Public variables:
	mpWindow*			m_Plot;				// graph window
	mpFXYVector*		dataLayer;			// vector data for graph

	double				totalfreq = 0.0;	// used for calculating... 
	int					freqcount = 0;		//			...average fundamental freq

private:
	// Private variables:
	// -- wxwidgets
	wxNotebook*			notebook;
	wxPanel*			spectropanel;
	wxPanel*			tunerpanel;
	wxButton*			startstopButton;
	kwxAngularMeter*	m_AngularMeter;		// needle gauge
	wxStaticText*		m_FreqText;			// label for displaying frequency
	wxStaticText*		m_NoteText;			// label for displaying musical note info
	wxTimer*			m_timer;
	wxTextCtrl*			m_Log;				// log window for the graph
	
	AudioCapturer*		capturer;
	boolean				running;
	double				frequency = MIN_FREQ;
	int					cents;	
	int					octave;				
	std::string			note;


	// wxWidgets macros:
	DECLARE_DYNAMIC_CLASS(AudioVisualizer)
	DECLARE_EVENT_TABLE()
};