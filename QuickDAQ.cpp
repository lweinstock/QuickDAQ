#include "QuickDAQ.hh"

#include <labdev/exceptions.hh>

using namespace std;

/*
 *      M A I N   F R A M E
 */

MainFrame::MainFrame(const wxString &title)
    : wxFrame(NULL, wxID_ANY, title, wxDefaultPosition, wxSize(1400,800), 
        wxDEFAULT_FRAME_STYLE | wxWANTS_CHARS | wxMAXIMIZE), 
      m_timerSweep(this, wxID_ANY), m_tree(), m_file()
{
    // TTree setup
    m_tree.Branch("vamp", &m_vamp);
    m_tree.Branch("freq", &m_freq);

    wxBoxSizer* globalSizer = new wxBoxSizer(wxHORIZONTAL);
    this->SetSizer(globalSizer);

    // Menu bar => TODO!
    // Status bar => TODO!

    // Left part: settings
    wxBoxSizer* bSizerLeft = new wxBoxSizer(wxVERTICAL);
    globalSizer->Add(bSizerLeft, 0, wxALL | wxEXPAND, 5);

    wxGridSizer* gSizerUpper = new wxGridSizer(2, 5, 5);
    bSizerLeft->Add(gSizerUpper, 0, wxALL | wxEXPAND, 5);

    // Osci setup
    wxStaticText* txtOsciIP = new wxStaticText(this, wxID_ANY, "Oscilloscope IP: ");
    wxStaticText* txtOsciPort = new wxStaticText(this, wxID_ANY, "Oscilloscope Port: ");
    m_tcOsciIP = new wxTextCtrl(this, wxID_ANY, "192.168.2.101");
    m_tcOsciPort = new wxTextCtrl(this, wxID_ANY, "5555");
    wxButton* btnOsciConnect = new wxButton(this, BTN_OSCI_CONN, "Connect");
    gSizerUpper->Add(txtOsciIP, 0, wxALL | wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT, 5);
    gSizerUpper->Add(m_tcOsciIP, 0, wxALL | wxEXPAND, 5);
    gSizerUpper->Add(txtOsciPort, 0, wxALL | wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT, 5);
    gSizerUpper->Add(m_tcOsciPort, 0, wxALL | wxEXPAND, 5);
    gSizerUpper->Add(0, 0, 1, wxEXPAND, 0); // Spacer
    gSizerUpper->Add(btnOsciConnect, 0, wxALL | wxEXPAND, 5);

    // Osci setup
    wxStaticText* txtFGenIP = new wxStaticText(this, wxID_ANY, "Function Generator IP: ");
    wxStaticText* txtFGenPort = new wxStaticText(this, wxID_ANY, "Function Generator Port: ");
    m_tcFGenIP = new wxTextCtrl(this, wxID_ANY, "192.168.2.102");
    m_tcFGenPort = new wxTextCtrl(this, wxID_ANY, "5555");
    wxButton* btnFGenConnect = new wxButton(this, BTN_FGEN_CONN, "Connect");
    gSizerUpper->Add(txtFGenIP, 0, wxALL | wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT, 5);
    gSizerUpper->Add(m_tcFGenIP, 0, wxALL | wxEXPAND, 5);
    gSizerUpper->Add(txtFGenPort, 0, wxALL | wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT, 5);
    gSizerUpper->Add(m_tcFGenPort, 0, wxALL | wxEXPAND, 5);
    gSizerUpper->Add(0, 0, 1, wxEXPAND, 0); // Spacer
    gSizerUpper->Add(btnFGenConnect, 0, wxALL | wxEXPAND, 5);

    // Mid part: Measurement Settings
    m_propGrid = new wxPropertyGrid(this, wxID_ANY, wxDefaultPosition, 
        wxDefaultSize, wxPG_SPLITTER_AUTO_CENTER | wxPG_DEFAULT_STYLE);
    bSizerLeft->Add(m_propGrid, 1, wxALL | wxEXPAND, 5);
    m_propGrid->Append(new wxPropertyCategory("Acquisition Settings"));
    m_propGrid->Append(new wxUIntProperty("Number of points", "nPts", wxGetApp().DAQSettings.nPoints));
    m_propGrid->Append(new wxFloatProperty("Start frequency [Hz]", "fSta", wxGetApp().DAQSettings.fStart));
    m_propGrid->Append(new wxFloatProperty("Stop frequency [Hz]", "fSto", wxGetApp().DAQSettings.fStop));
    m_propGrid->Append(new wxFloatProperty("Amplitude [V]", "ampl", wxGetApp().DAQSettings.vAmplitude));
    m_propGrid->Append(new wxStringProperty("Output file name", "fName", wxGetApp().DAQSettings.fileName));

    // Lower part: Start/Stop measurement
    wxBoxSizer* bSizerLower = new wxBoxSizer(wxHORIZONTAL);
    bSizerLeft->Add(bSizerLower, 0, wxALL | wxEXPAND, 5);
    wxButton* btnStart = new wxButton(this, BTN_START, "Start");
    wxButton* btnStop = new wxButton(this, BTN_STOP, "Stop");
    bSizerLower->Add(btnStart, 1, wxALL | wxEXPAND, 5);
    bSizerLower->Add(btnStop, 1, wxALL | wxEXPAND, 5);

    // Right part: log
    m_txtLog = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, 
        wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);
    globalSizer->Add(m_txtLog, 1, wxALL | wxEXPAND, 5);
    m_log = new wxLogStderr();
    wxLog::SetActiveTarget( new wxLogTextCtrl(m_txtLog) );

    this->Layout();

    // Bindings
    this->Bind(wxEVT_TIMER, &MainFrame::OnTimerUpdate, this);
    this->Bind(wxEVT_BUTTON, &MainFrame::OnButtonOsciConnect, this, BTN_OSCI_CONN);
    this->Bind(wxEVT_BUTTON, &MainFrame::OnButtonFGenConnect, this, BTN_FGEN_CONN);
    this->Bind(wxEVT_BUTTON, &MainFrame::OnButtonStart, this, BTN_START);
    this->Bind(wxEVT_BUTTON, &MainFrame::OnButtonStop, this, BTN_STOP);
    this->Bind(wxEVT_PG_CHANGED, &MainFrame::OnSettingChange, this);

    // Start 500ms timer
    //m_timerSweep.Start(500);
    return;
}

MainFrame::~MainFrame()
{
    wxLog::SetActiveTarget(nullptr);
    delete m_log;

    return;
}

void MainFrame::OnTimerUpdate(wxTimerEvent &ev)
{
    // Take measurement
    while ( !wxGetApp().osci.triggered() ) { usleep(100e3); }
    vector<double> time, volt;
    wxGetApp().osci.read_sample_data(1, time, volt);
    m_vamp = wxGetApp().osci.get_meas(1, labdev::osci::VAMP);
    wxLogMessage("Sample %i: VAmpl = %.2f V", m_counter, m_vamp);
    m_tree.Fill();

    // Prepare next measurement
    m_counter++;
    if (m_counter == m_freqSweep.size()) {
        m_timerSweep.Stop();
        m_counter = 0;
        m_tree.Write();
        //m_file.Write();
        m_file.Close();
        return;
    }
   
    m_freq = m_freqSweep.at(m_counter);
    wxGetApp().fgen.set_freq(1, m_freq);
    wxGetApp().osci.set_horz_base(0.25/m_freq);  // One period in four divisions
    return;
}

void MainFrame::OnButtonOsciConnect(wxCommandEvent &ev)
{
    string ip_addr = string(m_tcOsciIP->GetValue().mb_str());
    unsigned port = wxAtoi(m_tcOsciPort->GetValue());
    wxLogMessage("Connecting to oscillscope %s:%u...", ip_addr.c_str(), port);
    try {
        wxGetApp().OsciComm.open(ip_addr, port);
    } catch (labdev::exception &ex) {
        wxLogMessage("Failed to connect: %s", ex.what());
        return;
    }
    wxGetApp().osci.connect(&wxGetApp().OsciComm);
    wxLogMessage("Connected to %s", wxGetApp().osci.get_info());
    return;
}

void MainFrame::OnButtonFGenConnect(wxCommandEvent &ev)
{
    string ip_addr = string(m_tcFGenIP->GetValue().mb_str());
    unsigned port = wxAtoi(m_tcFGenPort->GetValue());
    wxLogMessage("Connecting to function generator %s:%u...", ip_addr.c_str(), port);
    try {
        wxGetApp().FGenComm.open(ip_addr, port);
    } catch (labdev::exception &ex) {
        wxLogMessage("Failed to connect: %s", ex.what());
        return;
    }
    wxGetApp().fgen.connect(&wxGetApp().FGenComm);
    wxLogMessage("Connected to %s", wxGetApp().fgen.get_info());
    return;
}

void MainFrame::OnButtonStart(wxCommandEvent &ev)
{
    // Sweep setup
    float fsta = wxGetApp().DAQSettings.fStart;
    float fsto = wxGetApp().DAQSettings.fStop;
    unsigned n =  wxGetApp().DAQSettings.nPoints;
    float step = (fsto - fsta)/(n - 1);
    for (unsigned i = 0; i < n; i++)
        m_freqSweep.push_back(fsta + step * i);

    // Function generator setup
    wxGetApp().fgen.set_wvfm(1, labdev::fgen::SINE);
    wxGetApp().fgen.set_ampl(1, wxGetApp().DAQSettings.vAmplitude);
    wxGetApp().fgen.set_offset(1, 0);
    wxGetApp().fgen.set_phase(1, 0);    
    wxGetApp().fgen.set_freq(1, fsta);
    wxGetApp().fgen.enable_channel(1);

    // Oscilloscope setup
    wxGetApp().osci.set_horz_base(0.25/fsta);

    // Create output file
    if (m_file.IsOpen())
        m_file.Close();
    m_file.Open(wxGetApp().DAQSettings.fileName.c_str(), "RECREATE");

    m_counter = 0;
    m_freq = fsta;
    m_timerSweep.Start(1000);

    return;
}

void MainFrame::OnButtonStop(wxCommandEvent &ev)
{
    return;
}

void MainFrame::OnSettingChange(wxPropertyGridEvent &ev)
{
    wxPGProperty* prop = ev.GetProperty();
    wxString name = prop->GetName();
    wxString label = prop->GetLabel();
    wxVariant value = prop->GetValue();

    // Dont handle unspecified values
    if (value.IsNull())
        return; 
    wxLogMessage("Changed '%s' to '%s'", label, value.GetString());

    if (name == "nPts") {
        wxGetApp().DAQSettings.nPoints = value.GetLong();
    } else if (name == "fSta") {
        wxGetApp().DAQSettings.fStart = value.GetDouble();
    } else if (name == "fSto") {
        wxGetApp().DAQSettings.fStop = value.GetDouble();
    } else if (name == "ampl") {
        wxGetApp().DAQSettings.vAmplitude = value.GetDouble();
    } else if (name == "fName") {
        wxGetApp().DAQSettings.fileName = value.GetString();
    }

    return;
}

/*
 *      A P P
 */

IMPLEMENT_APP(QuickDAQ)

bool QuickDAQ::OnInit()
{
    m_mainFrame = new MainFrame(_T("QuickDAQ"));
    m_mainFrame->Show(true);
    return true;
}

bool QuickDAQ::OnExceptionInMainLoop()
{
    try { 
        throw; 
    } catch(labdev::exception &ex) {
        wxMessageBox(ex.what(), "C++ Exception Caught", wxOK);
    }
    return true;
}