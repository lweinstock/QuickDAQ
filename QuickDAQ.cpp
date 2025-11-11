#include "QuickDAQ.hh"
#include "labkit/comms/tcpipcomm.hh"
#include "labkit/devices/functiongenerator.hh"
#include "labkit/devices/oscilloscope.hh"
#include "wx/log.h"
#include <labkit/exceptions.hh>

#include <memory>
#include <mutex>
#include <sstream>
#include <chrono>
#include <thread>


using namespace labkit;
using namespace std;

/*
 *      M A I N   F R A M E
 */
 
MainFrame::MainFrame(const wxString &title)
    : wxFrame(NULL, wxID_ANY, title, wxDefaultPosition, wxSize(1400,800), 
        wxDEFAULT_FRAME_STYLE | wxWANTS_CHARS), m_timerRefresh(this, wxID_ANY)
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
    m_btnOsciConnect = new wxButton(this, BTN_OSCI_CONN, "Connect");
    m_chbOsciUsb = new wxCheckBox(this, CHB_OSCI_USB, "Use USB");
    gSizerUpper->Add(txtOsciIP, 0, wxALL | wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT, 5);
    gSizerUpper->Add(m_tcOsciIP, 0, wxALL | wxEXPAND, 5);
    gSizerUpper->Add(txtOsciPort, 0, wxALL | wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT, 5);
    gSizerUpper->Add(m_tcOsciPort, 0, wxALL | wxEXPAND, 5);
    gSizerUpper->Add(m_chbOsciUsb, 0, wxALL | wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT, 5);
    gSizerUpper->Add(m_btnOsciConnect, 0, wxALL | wxEXPAND, 5);

    // Osci setup
    wxStaticText* txtFGenIP = new wxStaticText(this, wxID_ANY, "Function Generator IP: ");
    wxStaticText* txtFGenPort = new wxStaticText(this, wxID_ANY, "Function Generator Port: ");
    m_tcFGenIP = new wxTextCtrl(this, wxID_ANY, "192.168.2.102");
    m_tcFGenPort = new wxTextCtrl(this, wxID_ANY, "5555");
    m_btnFGenConnect = new wxButton(this, BTN_FGEN_CONN, "Connect");
    m_chbFGenUsb = new wxCheckBox(this, CHB_FGEN_USB, "Use USB");
    gSizerUpper->Add(txtFGenIP, 0, wxALL | wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT, 5);
    gSizerUpper->Add(m_tcFGenIP, 0, wxALL | wxEXPAND, 5);
    gSizerUpper->Add(txtFGenPort, 0, wxALL | wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT, 5);
    gSizerUpper->Add(m_tcFGenPort, 0, wxALL | wxEXPAND, 5);
    gSizerUpper->Add(m_chbFGenUsb, 0, wxALL | wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT, 5);
    gSizerUpper->Add(m_btnFGenConnect, 0, wxALL | wxEXPAND, 5);

    // Mid part: Measurement Settings
    m_propGrid = new wxPropertyGrid(this, wxID_ANY, wxDefaultPosition, 
        wxDefaultSize, wxPG_SPLITTER_AUTO_CENTER | wxPG_DEFAULT_STYLE);
    bSizerLeft->Add(m_propGrid, 1, wxALL | wxEXPAND, 5);
    m_propGrid->Append(new wxPropertyCategory("Acquisition Settings"));
    m_propGrid->Append(new wxUIntProperty("Number of points", "nPts", wxGetApp().DAQSettings.nPoints));
    m_propGrid->Append(new wxFloatProperty("Start frequency [Hz]", "fSta", wxGetApp().DAQSettings.fStart));
    m_propGrid->Append(new wxFloatProperty("Stop frequency [Hz]", "fSto", wxGetApp().DAQSettings.fStop));
    m_propGrid->Append(new wxFloatProperty("Horiz. scale factor", "horzScaleFactor", wxGetApp().DAQSettings.horzScaleFactor));
    m_propGrid->Append(new wxFloatProperty("Amplitude [V]", "ampl", wxGetApp().DAQSettings.vAmplitude));
    m_propGrid->Append(new wxStringProperty("Output file name", "fName", wxGetApp().DAQSettings.fileName));
    m_propGrid->Append(new wxBoolProperty("Quick DAQ", "quick", wxGetApp().DAQSettings.quickDAQ));

    // Lower part: Start/Stop measurement
    wxBoxSizer* bSizerLower = new wxBoxSizer(wxHORIZONTAL);
    bSizerLeft->Add(bSizerLower, 0, wxALL | wxEXPAND, 5);
    wxButton* btnStart = new wxButton(this, BTN_START, "Start");
    wxButton* btnStop = new wxButton(this, BTN_STOP, "Stop");
    bSizerLower->Add(btnStart, 1, wxALL | wxEXPAND, 5);
    bSizerLower->Add(btnStop, 1, wxALL | wxEXPAND, 5);

    // Right part: plot, progress and log
    wxBoxSizer* bSizerRight = new wxBoxSizer(wxVERTICAL);
    globalSizer->Add(bSizerRight, 1, wxALL | wxEXPAND, 5);

    // Plot
    m_plot = new wxPlot(this, m_time, m_volt);
    bSizerRight->Add(m_plot, 2, wxALL | wxEXPAND, 5);
    m_plot->SetRangeX(0, 10);
    m_plot->SetRangeY(-10, +10);
    m_plot->SetTitleX("Time [s]");
    m_plot->SetTitleY("Amplitude [V]");

    // Progress bar
    m_progress = new wxGauge(this, wxID_ANY, 10);
    bSizerRight->Add(m_progress, 0, wxALL | wxEXPAND, 5);

    // Log
    wxFont mono(12, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    m_txtLog = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, 
        wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);
    m_txtLog->SetFont(mono);
    m_log = new wxLogStderr();
    wxLog::SetActiveTarget( new wxLogTextCtrl(m_txtLog) );
    bSizerRight->Add(m_txtLog, 1, wxALL | wxEXPAND, 5);

    this->Layout();

    // Bindings
    this->Bind(wxEVT_TIMER, &MainFrame::OnTimerUpdate, this);
    this->Bind(wxEVT_BUTTON, &MainFrame::OnButtonConnectOsci, this, BTN_OSCI_CONN);
    this->Bind(wxEVT_BUTTON, &MainFrame::OnButtonConnectFGen, this, BTN_FGEN_CONN);
    this->Bind(wxEVT_BUTTON, &MainFrame::OnButtonStart, this, BTN_START);
    this->Bind(wxEVT_BUTTON, &MainFrame::OnButtonStop, this, BTN_STOP);
    this->Bind(wxEVT_PG_CHANGED, &MainFrame::OnSettingChange, this);
    this->Bind(wxEVT_CHECKBOX, &MainFrame::OnCheckOsciUsb, this, CHB_OSCI_USB);
    this->Bind(wxEVT_CHECKBOX, &MainFrame::OnCheckFGenUsb, this, CHB_FGEN_USB);

    return;
}



MainFrame::~MainFrame()
{
    m_stop_daq = false;
    if (m_daq_thread.joinable())
        m_daq_thread.join();

    wxLog::SetActiveTarget(nullptr);
    delete m_log;

    return;
}

void MainFrame::OnButtonConnectOsci(wxCommandEvent &ev)
{
    lock_guard<mutex> lock(m_osci_mutex);
    auto& osci = wxGetApp().GetOsci();

    if ( osci.connected() )
    {
        osci.disconnect();
        wxLogMessage("Disconnected osci: %s", osci.getInfo());
        m_tcOsciIP->Enable(true);
        m_tcOsciPort->Enable(true);
        m_chbOsciUsb->Enable(true);
        m_btnOsciConnect->SetLabel("Connect");
        return;
    }

    try 
    {
        if ( m_chbOsciUsb->IsChecked() )
        {
            auto comm = make_unique<UsbTmcComm>(Ds1000Z::VID, Ds1000Z::PID);
            osci.connect(std::move(comm));
        }
        else
        {
            string ip_addr = string(m_tcOsciIP->GetValue().mb_str());
            unsigned port = wxAtoi(m_tcOsciPort->GetValue());
            auto comm = make_unique<TcpipComm>(ip_addr, port);
            osci.connect(std::move(comm));
        }
    }
    catch (const Exception &ex) 
    {
        wxLogMessage("Failed to connect: '%s'", ex.what());
        osci.disconnect();
        return;
    }

    if ( osci.connected() )
    {
        wxLogMessage("Connected to %s", osci.getInfo());
        m_tcOsciIP->Enable(false);
        m_tcOsciPort->Enable(false);
        m_chbOsciUsb->Enable(false);
        m_btnOsciConnect->SetLabel("Disconnect");
    }

    return;
}

void MainFrame::OnButtonConnectFGen(wxCommandEvent &ev)
{
    lock_guard<mutex> lock(m_osci_mutex);
    auto& fgen = wxGetApp().GetFGen();

    if ( fgen.connected() )
    {
        fgen.disconnect();
        wxLogMessage("Disconnected fgen: %s", fgen.getInfo());
        m_tcFGenIP->Enable(true);
        m_tcFGenPort->Enable(true);
        m_chbFGenUsb->Enable(true);
        m_btnFGenConnect->SetLabel("Connect");
        return;
    }

    try 
    {
        if ( m_chbFGenUsb->IsChecked() )
        {
            auto comm = make_unique<UsbTmcComm>(Ds1000Z::VID, Ds1000Z::PID);
            fgen.connect(std::move(comm));
        }
        else
        {
            string ip_addr = string(m_tcFGenIP->GetValue().mb_str());
            unsigned port = wxAtoi(m_tcFGenPort->GetValue());
            auto comm = make_unique<TcpipComm>(ip_addr, port);
            fgen.connect(std::move(comm));
        }
    }
    catch (const Exception &ex) 
    {
        wxLogMessage("Failed to connect: '%s'", ex.what());
        fgen.disconnect();
        return;
    }

    if ( fgen.connected() )
    {
        wxLogMessage("Connected to %s", fgen.getInfo());
        m_tcFGenIP->Enable(false);
        m_tcFGenPort->Enable(false);
        m_chbFGenUsb->Enable(false);
        m_btnFGenConnect->SetLabel("Disconnect");
    }

    return;
}

void MainFrame::OnButtonStart(wxCommandEvent &ev)
{
    // If the thread is already running, stop it...
    m_stop_daq = true;
    if (m_daq_thread.joinable())
        m_daq_thread.join();

    // ... and start DAQ thread
    m_stop_daq = false;
    m_progress->SetRange(wxGetApp().DAQSettings.nPoints);
    m_progress->SetValue(0);
    m_daq_thread = thread(&MainFrame::DataAcquisition, this, wxGetApp().DAQSettings);

    m_timerRefresh.Start(100);
    return;
}

void MainFrame::OnButtonStop(wxCommandEvent &ev)
{
    m_stop_daq = true;
    m_timerRefresh.Stop();
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
    } else if (name == "nPerDiv") {
        wxGetApp().DAQSettings.horzScaleFactor = value.GetLong();
    } else if (name == "fName") {
        wxGetApp().DAQSettings.fileName = value.GetString();
    } else if (name == "quick") {
        wxGetApp().DAQSettings.quickDAQ = value.GetBool();
    }

    return;
}


void MainFrame::OnTimerUpdate(wxTimerEvent &ev)
{
    if (m_new_data)
    {
        {
            lock_guard<mutex> lock(m_data_mutex);
            m_plot->SetData(m_time, m_volt);
        }
        m_plot->AutoRange();
        m_plot->Refresh();
        m_new_data = false;
    }
    return;
}

void MainFrame::OnCheckOsciUsb(wxCommandEvent &ev)
{
    m_tcOsciIP->Enable(!ev.IsChecked());
    m_tcOsciPort->Enable(!ev.IsChecked());
    return;
}

void MainFrame::OnCheckFGenUsb(wxCommandEvent &ev)
{
    m_tcFGenIP->Enable(!ev.IsChecked());
    m_tcFGenPort->Enable(!ev.IsChecked());
    return;
}

void MainFrame::DataAcquisition(const Settings &s)
{
    // Prepare root file
    TFile outFile(s.fileName.c_str(), "RECREATE");
    TTree outTree("dataTree", "data");
    vector<double> volt, time;
    double vpp, vrms, vamp, freq;
    outTree.Branch("vpp", &vpp);
    outTree.Branch("vamp", &vamp);
    outTree.Branch("vrms", &vrms);
    outTree.Branch("freq", &freq);
    outTree.Branch("time", &time);
    outTree.Branch("voltage", &volt);
    // Result plots
    TGraph grVppVsFreq;
    TGraph grVampVsFreq;
    TGraph grVrmsVsFreq;
    grVppVsFreq.SetName("VppVsFreq");
    grVampVsFreq.SetName("VampVsFreq");
    grVrmsVsFreq.SetName("VrmsVsFreq");
    grVppVsFreq.SetTitle("VPP vs. Frequency;f [Hz];VPP [V]");
    grVampVsFreq.SetTitle("VAMP vs. Frequency;f [Hz];VAMP [V]");
    grVrmsVsFreq.SetTitle("VRMS vs. Frequency;f [Hz];VRMS [V]");

    // Calculate log sweep steps
    vector<double> sweep;
    float step = (log10(s.fStop) - log10(s.fStart))/(s.nPoints-1);
    for (unsigned i = 0; i < s.nPoints; i++) 
    {
        float f = pow(10, step * i + log10(s.fStart));
        sweep.push_back(f);
    }

    // Function generator setup
    try 
    {
        lock_guard<mutex> lock_fgen(m_fgen_mutex);
        auto& fgen = wxGetApp().GetFGen();
        fgen.setAmplitude(m_fgen_chan, s.vAmplitude);
        fgen.setWaveform(m_fgen_chan, FunctionGenerator::SINE);
        fgen.setOffset(m_fgen_chan, 0.);
        fgen.setPhase(m_fgen_chan, 0.);
        fgen.enableChannel(m_fgen_chan);
    }
    catch (Exception &ex)
    {
        wxLogMessage("Failed to configure function generator: %s", ex.what());
        return;
    }

    
    wxLogMessage("Starting DAQ (%u steps, f = %.3e - %.3eHz) ...", sweep.size(), 
        *sweep.begin(), *(sweep.end() - 1));

    int progress = 0;
    for (auto f : sweep)
    {
        if (m_stop_daq)
        {
            wxLogMessage("Stopping DAQ...");
            break;
        }

        try 
        {
            unique_lock<mutex> lock_fgen(m_fgen_mutex);
            auto& fgen = wxGetApp().GetFGen();
            fgen.setFrequency(m_fgen_chan, f);
            lock_fgen.unlock();
            wxLogMessage("Setting frequency to %.3e Hz", f);

            unique_lock<mutex> lock_osci(m_osci_mutex);
            auto& osci = wxGetApp().GetOsci();
            osci.setHorzBase(s.horzScaleFactor/f);  // Apply correct scale

            // Adjust vertical scale
            osci.run();
            this_thread::sleep_for(chrono::milliseconds(100));

            double vert_base = osci.getVertBase(m_osci_chan);
            vpp = osci.getMeasurement(m_osci_chan, Oscilloscope::VPP);
            
            // If voltage is too low, decrease scale
            while ( vpp < 4*vert_base )
            {
                wxLogMessage("VPP too small (%.3f), vert %.3f -> %.3f", vpp,
                    vert_base, 0.7*vert_base);
                osci.setVertBase(m_osci_chan, 0.7*vert_base);
                this_thread::sleep_for(chrono::milliseconds(500));
                vpp = osci.getMeasurement(m_osci_chan, Oscilloscope::VPP);
                vert_base = osci.getVertBase(m_osci_chan);
            }

            // If voltage is too large, increase scale
            while ( vpp > 6*vert_base )
            {
                wxLogMessage("VPP too large (%.3f), vert %.3f -> %.3f", vpp,
                    vert_base, 1.3*vert_base);
                osci.setVertBase(m_osci_chan, 1.3*vert_base);
                this_thread::sleep_for(chrono::milliseconds(500));
                vpp = osci.getMeasurement(m_osci_chan, Oscilloscope::VPP);
                vert_base = osci.getVertBase(m_osci_chan);
            }

            if (!s.quickDAQ)
            {
                osci.singleShot();
                while (!osci.stopped())
                    this_thread::sleep_for(chrono::milliseconds(100));
            }

            this_thread::sleep_for(chrono::milliseconds(500));
            vpp = osci.getMeasurement(m_osci_chan, Oscilloscope::VPP);
            vamp = osci.getMeasurement(m_osci_chan, Oscilloscope::VAMP);
            vrms = osci.getMeasurement(m_osci_chan, Oscilloscope::VRMS);
            osci.readSampleData(m_osci_chan, volt, time);
            lock_osci.unlock();
            
            // Write data to output file
            freq = f;
            grVppVsFreq.AddPoint(freq, vpp);
            grVrmsVsFreq.AddPoint(freq, vrms);
            grVampVsFreq.AddPoint(freq, vamp);
            outTree.Fill();
            // Update progress bar
            progress++;
            m_progress->SetValue(progress);

            wxLogMessage("[%i] Read %lu points of data (VPP=%.3fV/VAMP=%.3fV/"
                "VRMS=%.3fV)", progress, time.size(), vpp, vamp, vrms);
        }
        catch (const Exception &ex)
        {
            wxLogMessage("Failed to read data: %s", ex.what());
            m_stop_daq = false; 
            break;
        }

        lock_guard<mutex> lock(m_data_mutex);
        m_volt = volt;
        m_time = time;
        m_new_data = true;
    }

    grVppVsFreq.Write();
    grVrmsVsFreq.Write();
    grVampVsFreq.Write();
    outFile.Write();
    outFile.Close();
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
    } catch(const Exception &ex) {
        wxMessageBox(ex.what(), "C++ Exception Caught", wxOK);
    }
    return true;
}