/**
* @file		AudioVisualizer.cpp
* @author	Justin Hoggart <jwhoggart@gmail.com>
* @version	1.0
*
* The AudioVisualizer class is responsible for the visualization of the
* audio data.
**/

#include "AudioVisualizer.h"

// Unique identifiers:
enum {
	ID_QUIT = wxID_HIGHEST,
	ID_BOOKCTRL,
	ID_ANGULAR_METER,
	ID_STARTSTOP_BUTTON,
	ID_REFRESH_TIMER
};

// wxWidgets macros:
IMPLEMENT_DYNAMIC_CLASS(AudioVisualizer, wxFrame)
// -- event table
BEGIN_EVENT_TABLE(AudioVisualizer, wxFrame)
	EVT_MENU(ID_QUIT, AudioVisualizer::OnQuit)
	EVT_MENU(mpID_FIT, AudioVisualizer::OnFit)
	EVT_BUTTON(ID_STARTSTOP_BUTTON, AudioVisualizer::OnStartStopButton)
	EVT_CLOSE(AudioVisualizer::OnClose)
	EVT_TIMER(ID_REFRESH_TIMER, AudioVisualizer::OnRefreshTimer)
END_EVENT_TABLE()

// ****** Constructor:
AudioVisualizer::AudioVisualizer() : wxFrame((wxFrame *)NULL, -1, wxT("RT-Tuner"), wxDefaultPosition, wxSize(500, 500)) 
{
	// Menus:
	wxMenu *fileMenu = new wxMenu();

	fileMenu->Append(ID_QUIT, wxT("E&xit\tAlt-X"));

	wxMenuBar *menu_bar = new wxMenuBar();
	menu_bar->Append(fileMenu, wxT("&File"));

	SetMenuBar(menu_bar);
	CreateStatusBar(1);
	SetStatusText(wxT(""), 0);

	// Main window:
	wxBoxSizer* windowsizer = new wxBoxSizer(wxVERTICAL);

	// -- Notebook widget
	notebook = new wxNotebook(this, ID_BOOKCTRL, wxDefaultPosition, wxDefaultSize, 0 );
	
	// ---- Spectrum tab
	spectropanel = new wxPanel(notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
	wxBoxSizer* spectrosizer = new wxBoxSizer(wxVERTICAL);

	// ------ initialize the dataLayer vector
	std::vector<double> spectrum;
	std::vector<double> scale;
	dataLayer = new mpFXYVector();
	
	for (size_t i = 0; i < AUDIO_BUFFER_FRAMES; i += 1) {
		spectrum.push_back(0);
		scale.push_back(i);
	}

	dataLayer->SetData(scale, spectrum);
	dataLayer->SetContinuity(true);
	wxPen dataPen(*wxBLUE, 2, wxSOLID);
	dataLayer->SetPen(dataPen);
	dataLayer->SetDrawOutsideMargins(false);
	
	// ------ create and configure the graph
	m_Plot = new mpWindow(spectropanel, -1, wxPoint(0, 0), wxSize(100, 100), wxSUNKEN_BORDER);
	mpScaleX* xaxis = new mpScaleX(wxT("frequency"), mpALIGN_CENTER, TRUE, mpX_NORMAL);
	mpScaleY* yaxis = new mpScaleY(wxT("magnitude"), mpALIGN_CENTER, TRUE);
	m_Plot->AddLayer(xaxis);
	m_Plot->AddLayer(yaxis);
	m_Plot->AddLayer(dataLayer);
	m_Plot->EnableDoubleBuffer(true);
	m_Plot->EnableMousePanZoom(false);
	m_Plot->SetMPScrollbars(false);
	m_Plot->Fit();

	// ------ create the log window
	m_Log = new wxTextCtrl(spectropanel, -1, wxT("---Program started---\n"), wxPoint(0, 0), wxSize(100, 100), wxTE_MULTILINE);
	wxLog *old_log = wxLog::SetActiveTarget(new wxLogTextCtrl(m_Log));
	delete old_log;

	// ------ (layout organization)
	spectrosizer->Add(m_Plot, 1, wxALL | wxEXPAND, 5);
	spectrosizer->Add(m_Log, 0, wxALL | wxEXPAND, 5);

	spectropanel->SetSizer(spectrosizer);
	spectropanel->Layout();
	spectrosizer->Fit(spectropanel);
	
	notebook->AddPage(spectropanel, wxT("Spectrum"), true);

	// ---- Tuner tab
	tunerpanel = new wxPanel(notebook);
	wxBoxSizer* tunersizer = new wxBoxSizer(wxVERTICAL);

	// ------ configure the needle gauge and text labels
	AudioVisualizer::SetUpAngularMeter(tunerpanel);
	m_FreqText = new wxStaticText(tunerpanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_CENTER | wxTE_MULTILINE);
	m_NoteText = new wxStaticText(tunerpanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_CENTER | wxTE_MULTILINE);
	
	// ------ (layout organization)
	tunersizer->Add(m_AngularMeter, 0, wxALIGN_CENTER_HORIZONTAL);
	tunersizer->Add(m_FreqText, 1, wxALL | wxALIGN_CENTER_HORIZONTAL, 5);
	tunersizer->Add(m_NoteText, 1, wxALL | wxALIGN_CENTER_HORIZONTAL, 5);
	
	tunerpanel->SetSizer(tunersizer);
	tunerpanel->Layout();
	tunersizer->Fit(tunerpanel);
	
	notebook->AddPage(tunerpanel, wxT("Tuner"), false);

	windowsizer->Add(notebook, 1, wxEXPAND | wxALL, 5);

	// -- Start/stop button
	startstopButton = new wxButton(this, ID_STARTSTOP_BUTTON, wxT("&Stop"), wxDefaultPosition, wxDefaultSize, 0);
	startstopButton->Enable(true);
	
	// ----(layout organization)
	windowsizer->Add(startstopButton, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, 5);
	this->SetSizer(windowsizer);
	this->Layout();
	this->Centre(wxBOTH);
	this->RefreshWindow();

	// -- Timer
	m_timer = new wxTimer(this, ID_REFRESH_TIMER);
	m_timer->Start(REFRESH_INTERVAL);

	// initialize audio capturer
	capturer = new AudioCapturer(this->dataLayer, &this->totalfreq, &this->freqcount);
	this->running = true;	// indicate that the audio function is running

}

// ****** Methods:
// Initial configuration for the needle gauge widget
void AudioVisualizer::SetUpAngularMeter(wxPanel* panel) 
{
	// create the angular meter object	
	m_AngularMeter = new kwxAngularMeter(panel, ID_ANGULAR_METER, wxT("Angular Meter"), wxPoint(10, 20), wxSize(250, 250));

	// set the number of sectors
	m_AngularMeter->SetNumSectors(3);
	m_AngularMeter->SetNumTick(3);
	
	// define the scale
	m_AngularMeter->SetRange(MIN_FREQ, MAX_FREQ);
	m_AngularMeter->SetAngle(-20, 200);
	m_AngularMeter->SetValue(220);

	// define the colour scheme
	m_AngularMeter->SetSectorColor(0, *wxWHITE);
	m_AngularMeter->SetSectorColor(1, *wxWHITE);
	m_AngularMeter->SetSectorColor(2, *wxWHITE);
	m_AngularMeter->SetNeedleColour(*wxBLACK);
	m_AngularMeter->SetBackColour(*wxLIGHT_GREY);
	m_AngularMeter->SetBorderColour(*wxWHITE);
}

// Posts a message to the log window
void AudioVisualizer::WriteToGraphLog(wxString msg) 
{
	(*m_Log) << msg << "\n";
}

// Convert a frequency value to the corresponding note, octave, and cent info
void AudioVisualizer::FreqToNote(double freq) 
{
	std::string theNote;
	double lnote = (std::log(freq) - std::log(440)) / std::log(2) + 4.0;
	int theOctave = std::floor(lnote);
	int theCents = 1200 * (lnote - theOctave);
	double offset = 50.0;
	size_t x = 2;
	std::string notes = "A BbB C C#D EbE F F#G G#";

	// cents are on a 1200 pt cyclical scale, with "A" as the base note
	if (theCents < 50) {
		theNote = "A ";
	} 
	else if (theCents >= 1150) {
		theNote = "A ";
		theCents -= 1200;
		octave++;
	} 
	else {
		// every individual note covers a 100 cent range; determine which note is appropriate
		for (size_t j = 1; j <= 11; j += 1) {
			if (theCents >= offset && theCents < (offset + 100)) {
				theNote = notes.at(x);
				theNote += notes.at(x + 1);
				
				theCents -= (j * 100);
				break;
			}

			offset += 100;
			x += 2;
		}
	}
	
	// update the instance variables
	this->cents = theCents;
	this->octave = theOctave;
	this->note = theNote;
}

// Print the note/octave/cent info to their label
void AudioVisualizer::WriteNote() 
{
	m_NoteText->SetLabelText(this->note + std::to_string(this->octave) + "  \n(" + std::to_string(this->cents) + " cents)");
}

// Refresh the GUI window
void AudioVisualizer::RefreshWindow() 
{
	// When viewing the "Tuner" tab:
	if (this->notebook->GetCurrentPage() == this->tunerpanel) {
		// wait for at least five data points before determining a fundamental freq
		if (freqcount > 4) {
			this->frequency = (this->totalfreq / this->freqcount);
			this->totalfreq = 0.0;
			this->freqcount = 0;
		}

		// cap the frequency within the relevant range
		if (this->frequency > MAX_FREQ)
			m_AngularMeter->SetValue(MAX_FREQ);
		else if (this->frequency < MIN_FREQ)
			m_AngularMeter->SetValue(MIN_FREQ);
		else
			m_AngularMeter->SetValue(this->frequency);

		// display frequency, and determine corresponding note info
		m_FreqText->SetLabelText(std::to_string(this->frequency));
		this->FreqToNote(this->frequency);
		this->WriteNote();
	} 
	// When viewing the "Spectrum" tab:
	else if (this->notebook->GetCurrentPage() == this->spectropanel) {
		// update the graph
		this->frequency = (this->totalfreq / this->freqcount);
		this->m_Plot->Fit(-50,500,-50,30);
	}

	if (running)
		this->SetStatusText("Audio stream running...");
	else
		this->SetStatusText("Audio stream stopped.");
}

// Start the audio stream
int AudioVisualizer::InitializeAudio() 
{
	if (this->capturer->InitializeAudio() == -1) {
		std::cout << "Error encountered. Exiting...\n";
		return -1;
	}

	return 0;
}

// ****** Event handlers:
//Handler for close via File -> Exit menu
void AudioVisualizer::OnQuit(wxCommandEvent &WXUNUSED(event))
{
	// stop capturing audio
	if (this->capturer->StopCapture() != 0) {
		exit(-1);
	}

	// close the open audio stream
	this->capturer->CloseStream();
	Close(TRUE);
}

// Handler for close via "X" button, etc
void AudioVisualizer::OnClose(wxCloseEvent& event)
{
	// stop capturing audio
	if (this->capturer->StopCapture() != 0) {
		exit(-1);
	}

	// close the open audio stream
	this->capturer->CloseStream();
	Destroy();
	Close(TRUE);
}

// Handler for auto-fitting the graph
void AudioVisualizer::OnFit(wxCommandEvent &WXUNUSED(event))
{
	// auto-fit the graph data
	m_Plot->Fit();
}

// Handler for audio stream start/stop button
void AudioVisualizer::OnStartStopButton(wxCommandEvent& WXUNUSED(event))
{
	if (running) {
		// try to stop the audio capture
		if (this->capturer != NULL) {
			if (this->capturer->StopCapture() != 0) {
				this->WriteToGraphLog("***Problem stopping the audio stream");
			}
			else {
				running = false;
				this->startstopButton->SetLabel("Start");
				this->WriteToGraphLog("Audio stream stopped.");
			}
		}
	}
	else {
		// try to start capturing audio
		if (this->capturer->StartCapture() != 0) {
			this->WriteToGraphLog("***Problem starting the audio stream");
		}
		else {
			running = true;
			this->startstopButton->SetLabel("Stop");
			this->WriteToGraphLog("Audio stream started.");
		}
	}
}

// Handler for timer
void AudioVisualizer::OnRefreshTimer(wxTimerEvent& event)
{
	this->RefreshWindow();
}

