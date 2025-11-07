#ifndef LABDAQ_HH
#define LABDAQ_HH

#include <condition_variable>
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif
#include <wx/propgrid/propgrid.h>
#include "plot.hh"

#include <TFile.h>
#include <TTree.h>
#include <TGraph.h>

#include <labkit/devices/rigol/ds1000z.hh>
#include <labkit/devices/rigol/dg4000.hh>

#include <thread>
#include <mutex>

struct Settings {
    unsigned nPoints {10};
    float fStart {100e3};
    float fStop {10e6};
    float vAmplitude {20.};
    float horzScaleFactor {1.0};
    std::string fileName {"test.root"};
    bool quickDAQ {true};
};

class MainFrame : public wxFrame
{
public:
    MainFrame(const wxString &title);
    ~MainFrame();

    void OnButtonConnectOsci(wxCommandEvent &ev);
    void OnButtonConnectFGen(wxCommandEvent &ev);
    void OnButtonStart(wxCommandEvent &ev);
    void OnButtonStop(wxCommandEvent &ev);
    void OnSettingChange(wxPropertyGridEvent &ev);
    void OnTimerUpdate(wxTimerEvent &ev);
    void OnCheckOsciUsb(wxCommandEvent &ev);
    void OnCheckFGenUsb(wxCommandEvent &ev);

    void DataAcquisition(const Settings &s);

protected:
    wxTextCtrl* m_txtLog;
    wxTextCtrl* m_tcOsciIP;
    wxTextCtrl* m_tcOsciPort;
    wxCheckBox* m_chbOsciUsb;
    wxButton* m_btnOsciConnect;
    wxTextCtrl* m_tcFGenPort;
    wxTextCtrl* m_tcFGenIP;
    wxCheckBox* m_chbFGenUsb;
    wxButton* m_btnFGenConnect;
    wxPropertyGrid* m_propGrid;
    wxPlot* m_plot;

private:
    wxLog* m_log;
    wxTimer m_timerRefresh;

    unsigned m_osci_chan {1}, m_fgen_chan {1};
    std::vector<double> m_time, m_volt;

    // Multi-threading
    std::thread m_daq_thread;
    std::mutex m_osci_mutex {}, m_fgen_mutex{}, m_data_mutex {};
    std::atomic<bool> m_stop_daq {false}, m_new_data {false};
};

class QuickDAQ : public wxApp 
{
public:

    virtual bool OnInit() override;
    virtual void OnUnhandledException() override { throw; }
    virtual bool OnExceptionInMainLoop() override;

    Settings DAQSettings;

    auto& GetOsci() { return m_osci; }
    auto& GetFGen() { return m_fgen; }

protected:
    MainFrame* m_mainFrame;
    
private:
    labkit::Ds1000Z m_osci;
    labkit::Dg4000 m_fgen;
};

DECLARE_APP(QuickDAQ);    

// Event IDs
enum : int 
{
    FILE_QUIT = wxID_EXIT,
    HELP_ABOUT = wxID_ABOUT,
    BTN_OSCI_CONN = wxID_HIGHEST + 1,
    BTN_FGEN_CONN,
    BTN_START,
    BTN_STOP,
    CHB_OSCI_USB,
    CHB_FGEN_USB
};

#endif