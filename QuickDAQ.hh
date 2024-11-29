#ifndef LABDAQ_HH
#define LABDAQ_HH

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif
#include <wx/propgrid/propgrid.h>

#include <TFile.h>
#include <TTree.h>
#include <TGraph.h>

#include <labdev/tcpip_interface.hh>
#include <labdev/devices/osci.hh>
#include <labdev/devices/fgen.hh>

#include <memory>

class MainFrame : public wxFrame
{
public:
    MainFrame(const wxString &title);
    ~MainFrame();

    void OnButtonOsciConnect(wxCommandEvent &ev);
    void OnButtonFGenConnect(wxCommandEvent &ev);
    void OnButtonStart(wxCommandEvent &ev);
    void OnButtonStop(wxCommandEvent &ev);
    void OnSettingChange(wxPropertyGridEvent &ev);
    void OnTimerUpdate(wxTimerEvent &ev);

    void ReadTrace(std::vector<double> time, std::vector<double> volt);
    void StartMeasurement();
    void StopMeasurement();

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

    TTree* m_tree;  // Root objects with raw pointers, since root does its own 
    TFile* m_file;  // resource management/ownership ...
    TGraph* m_gr;

    float m_freq {0};
    float m_vpp {0}, m_vrms{0}, m_vamp{0};
    unsigned m_counter {0};
    unsigned m_osciChan {1}, m_fgenChan {1};
    std::vector<float> m_freqSweep {};
    std::vector<double> m_time, m_volt;
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
        unsigned nPerDiv {1};
        std::string fileName {"test.root"};
    } DAQSettings;

    void InitOsci(std::string ip, unsigned port);
    void InitFGen(std::string ip, unsigned port);
    std::shared_ptr<labdev::osci> GetOsci() { return m_osci; }
    std::shared_ptr<labdev::fgen> GetFGen() { return m_fgen; }

protected:
    MainFrame* m_mainFrame;
    
private:
    labdev::tcpip_interface m_osciComm;
    labdev::tcpip_interface m_fgenComm;
    std::shared_ptr<labdev::osci> m_osci{};
    std::shared_ptr<labdev::fgen> m_fgen{};
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