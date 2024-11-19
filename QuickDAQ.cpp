#include "QuickDAQ.hh"

#include <labdev/exceptions.hh>
#include <labdev/devices/rigol/ds1000z.hh>
#include <labdev/devices/rigol/dg4000.hh>

using namespace std;

/*
 *      M A I N   F R A M E
 */

MainFrame::MainFrame(const wxString &title)
    : wxFrame(NULL, wxID_ANY, title, wxDefaultPosition, wxSize(1400,800), 
        wxDEFAULT_FRAME_STYLE | wxWANTS_CHARS | wxMAXIMIZE), 
      m_timerSweep(this, wxID_ANY), m_tree(nullptr), m_file(nullptr)
{
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

    auto fgen_ptr = wxGetApp().GetFGen();
    auto osci_ptr = wxGetApp().GetOsci();

    // Set frequency and osci time base
    m_freq = m_freqSweep.at(m_counter);
    fgen_ptr->set_freq(m_fgenChan, m_freq);
    osci_ptr->set_horz_base(0.25/m_freq);

    // Wait until waveform has settled
    usleep(500e3);  // Replace by fgen->wait_to_complete() ?

    // Take measurement
    m_vpp = osci_ptr->get_meas(m_osciChan, labdev::osci::VPP);
    osci_ptr->read_sample_data(m_osciChan, m_volt, m_time);

    // Store data in tree
    m_tree->Fill();
    wxLogMessage("Meas %03i: f=%.3eHz, VPP=%.3fV, n=%lu", 
        m_counter, m_freq, m_vpp, m_volt.size());

    // Increase counter
    m_counter++;
    if ( m_counter == m_freqSweep.size() )
        this->StopMeasurement();

    return;
}

void MainFrame::OnButtonOsciConnect(wxCommandEvent &ev)
{
    string ip_addr = string(m_tcOsciIP->GetValue().mb_str());
    unsigned port = wxAtoi(m_tcOsciPort->GetValue());
    wxGetApp().InitOsci(ip_addr, port);
    return;
}

void MainFrame::OnButtonFGenConnect(wxCommandEvent &ev)
{

    string ip_addr = string(m_tcFGenIP->GetValue().mb_str());
    unsigned port = wxAtoi(m_tcFGenPort->GetValue());
    wxGetApp().InitFGen(ip_addr, port);
    return;
}

void MainFrame::OnButtonStart(wxCommandEvent &ev)
{
    // Function generator setup
    auto fgen_ptr = wxGetApp().GetFGen();
    fgen_ptr->set_wvfm(m_fgenChan, labdev::fgen::SINE);
    fgen_ptr->set_ampl(m_fgenChan, wxGetApp().DAQSettings.vAmplitude);
    fgen_ptr->set_offset(m_fgenChan, 0);
    fgen_ptr->set_phase(m_fgenChan, 0);    
    fgen_ptr->set_freq(m_fgenChan, wxGetApp().DAQSettings.fStart);
    fgen_ptr->enable_channel(1);

    // Oscilloscope setup
    auto osci_ptr = wxGetApp().GetOsci();
    osci_ptr->set_horz_base(0.25/wxGetApp().DAQSettings.fStart);

    this->StartMeasurement();
    return;
}

void MainFrame::OnButtonStop(wxCommandEvent &ev)
{
    this->StopMeasurement();
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

void MainFrame::ReadTrace(vector<double> time, vector<double> volt)
{
    auto osci_ptr = wxGetApp().GetOsci();
    while ( !osci_ptr->triggered() ) 
    {
        wxLogMessage("Waiting for trigger...");
        usleep(500e3);
    }
    time.clear();
    volt.clear();
    osci_ptr->read_sample_data(m_osciChan, time, volt);
    return;
}

void MainFrame::StartMeasurement()
{
    // Create output file
    if ( m_file && m_file->IsOpen() ) {
        m_file->Close();
        //delete m_file;
        //delete m_tree;
        //m_file = nullptr;
        //m_tree = nullptr;
    }
    m_file = new TFile(wxGetApp().DAQSettings.fileName.c_str(), "RECREATE");
    m_tree = new TTree("dataTree", "data");
    m_tree->Branch("vamp", &m_vpp);
    m_tree->Branch("freq", &m_freq);
    m_tree->Branch("time", &m_time);
    m_tree->Branch("voltage", &m_volt);

    // Sweep setup
    m_freqSweep.clear();
    m_counter = 0;
    float fsta = wxGetApp().DAQSettings.fStart;
    float fsto = wxGetApp().DAQSettings.fStop;
    unsigned n =  wxGetApp().DAQSettings.nPoints;
    //float step = (fsto - fsta)/(n - 1);
    float step = (log10(fsto) - log10(fsta))/(n-1);
    for (unsigned i = 0; i < n; i++) {
        float f = pow(10, step * i + log10(fsta));
        m_freqSweep.push_back(f);
    }

    // Start timer
    m_timerSweep.Start(1000);
    return;
}

void MainFrame::StopMeasurement()
{
    m_timerSweep.Stop();
    m_freqSweep.clear();
    m_counter = 0;

    if ( m_file && m_file->IsOpen() ) {
        m_tree->Write();
        m_file->Close();
        //delete m_file;
        //delete m_tree;
        //m_file = nullptr;
        //m_tree = nullptr;
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

void QuickDAQ::InitOsci(std::string ip, unsigned port)
{
    // Connect to device
    wxLogMessage("Connecting to oscillscope %s:%u...", ip.c_str(), port);
    try {
        if (m_osciComm.good()) 
            m_osciComm.close();
        m_osciComm.open(ip, port);
    } catch (labdev::exception &ex) {
        m_osciComm.close();
        wxLogMessage("Failed to connect: %s", ex.what());
        return;
    }
    m_osci = make_shared<labdev::ds1000z>(&m_osciComm);
    wxLogMessage("Connected to %s", m_osci->get_info());
    return;
}

void QuickDAQ::InitFGen(std::string ip, unsigned port)
{
    // Connect to device
    wxLogMessage("Connecting to function generator %s:%u...", ip.c_str(), port);
    try {
        if (m_fgenComm.good()) 
            m_fgenComm.close();
        m_fgenComm.open(ip, port);
    } catch (labdev::exception &ex) {
        m_fgenComm.close();
        wxLogMessage("Failed to connect: %s", ex.what());
        return;
    }
    m_fgen = make_shared<labdev::dg4000>(&m_fgenComm);
    wxLogMessage("Connected to %s", m_fgen->get_info());
    return;
}
