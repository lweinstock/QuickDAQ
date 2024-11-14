#ifndef LABDAQ_HH
#define LABDAQ_HH

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif
#include <wx/propgrid/propgrid.h>

#include <TFile.h>
#include <TTree.h>

#include <labdev/tcpip_interface.hh>
#include <labdev/devices/rigol/ds1000z.hh>
#include <labdev/devices/rigol/dg4000.hh>

#include <memory>

class MainFrame : public wxFrame
{
public:
    MainFrame(const wxString &title);
    ~MainFrame();

    void OnTimerUpdate(wxTimerEvent &ev);
    void OnButtonOsciConnect(wxCommandEvent &ev);
    void OnButtonFGenConnect(wxCommandEvent &ev);
    void OnButtonStart(wxCommandEvent &ev);
    void OnButtonStop(wxCommandEvent &ev);
    void OnSettingChange(wxPropertyGridEvent &ev);

protected:
    wxTextCtrl* m_txtLog;
    wxTextCtrl* m_tcOsciIP;
    wxTextCtrl* m_tcOsciPort;
    wxTextCtrl* m_tcFGenPort;
    wxTextCtrl* m_tcFGenIP;
    wxPropertyGrid* m_propGrid;

private:
    wxLog* m_log;
    wxTimer m_timerSweep;

    TTree m_tree;
    TFile m_file;

    float m_freq {0};
    float m_vamp {0};
    unsigned m_counter {0};
    std::vector<float> m_freqSweep {};
};

class QuickDAQ : public wxApp 
{
public:
    virtual bool OnInit() override;

    virtual void OnUnhandledException() override { throw; }
    virtual bool OnExceptionInMainLoop() override;

    struct {
        unsigned nPoints {10};
        float fStart {100e3};
        float fStop {10e6};
        float vAmplitude {20.};
        std::string fileName {"test.root"};
    } DAQSettings;

    labdev::tcpip_interface OsciComm;
    labdev::tcpip_interface FGenComm;
    labdev::ds1000z osci;
    labdev::dg4000 fgen;

protected:
    MainFrame* m_mainFrame;
    
private:
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
    BTN_STOP
};

#endif