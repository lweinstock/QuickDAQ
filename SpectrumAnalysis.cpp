#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#include <TFile.h>
#include <TTree.h>
#include <TGraph.h>
#include <TAxis.h>
#include <TCanvas.h>
#include <TPad.h>
#include <TH1.h>

#include <fftw3.h>

#include "waveform.hh"

using namespace std;

int main(int argc, char** argv)
{
    if (argc != 2) {
        cout << "Usage ./SpectrumAnalysis <input>.root" << endl;
        return -1;
    }

    // Open input file
    string inFName(argv[1]);

    TFile* inFile = new TFile(inFName.c_str(), "READ"); // Root just looooves pointers...
    TTree* inTree = (TTree*)inFile->Get("dataTree");
    float vpp, freq;
    std::vector<double>* pTime = 0;
    std::vector<double>* pVolt = 0;
    inTree->SetBranchAddress("vamp", &vpp);
    inTree->SetBranchAddress("freq", &freq);
    inTree->SetBranchAddress("time", &pTime);
    inTree->SetBranchAddress("voltage", &pVolt);

    // Create output file for storing the frequency spectra
    string outFName = inFName.substr(0, inFName.find(".root"));
    outFName += "_spectrum.root";
    TFile* outFile = new TFile(outFName.c_str(), "RECREATE");

    TTree* outTree = new TTree("outTree", "spectrumData");
    vector<double> freqFFT, magnFFT;
    outTree->Branch("freqFFT", &freqFFT);
    outTree->Branch("magnFFT", &magnFFT);

    TGraph* grMagn;
    TGraph* grPhase;
    TGraph* grTrans;
    TGraph* grRMSvsFreq = new TGraph();

    TCanvas* c1 = new TCanvas("", "Filter", 1200, 900);
    c1->Divide(1, 3);
    TPad* p1 = (TPad*)c1->cd(1);
    p1->SetLogx();
    p1->SetLogy();
    TPad* p2 = (TPad*)c1->cd(2);
    p2->SetLogx();

    // Read data in
    unsigned n = inTree->GetEntries();
    cout << "Found " << n << " entries" << endl;

    for (unsigned i = 0; i < n; i++)
    {
        inTree->GetEntry(i);
        cout << i << ": f=" << freq << ", vpp=" << vpp << endl;

        if (pTime->size() == 0) {
            cout << "Empty set" << endl;
            continue;
        }

        waveform transient(pTime->data(), pVolt->data(), pTime->size());
        //transient.applyFilter( waveform::gaus(1., freq, 0.2*freq) );
        //transient.applyFilter( waveform::butterworthLowpass(1., 1.2*freq, 5) );
        //transient.applyFilter( waveform::butterworthHighpass(1., 0.8*freq, 5) );
        transient.applyFilter( waveform::butterworthBandpass(1., 0.8*freq, 1.2*freq, 5) );

        waveform fftMagn, fftPhase;
        transient.getFFT(fftMagn, fftPhase);
        unsigned start = 1000;
        unsigned stop = transient.getSize() - 1000;
        grRMSvsFreq->AddPoint(freq, transient.getRMS(start, stop));

        grMagn = new TGraph(fftMagn.getSize(), fftMagn.getX().data(), fftMagn.getY().data());
        stringstream ssTitle;
        ssTitle << "Magnitude" << i << " fsig = " << freq << "Hz";
        grMagn->SetName(ssTitle.str().c_str()); 
        grMagn->SetTitle(ssTitle.str().c_str()); 
        grMagn->GetXaxis()->SetLimits(1e4, 1e8);
        grMagn->GetHistogram()->SetMinimum(1e-8);
        grMagn->GetHistogram()->SetMaximum(1);
        c1->cd(1);
        grMagn->Draw("APL");

        grPhase = new TGraph(fftPhase.getSize(), fftPhase.getX().data(), fftPhase.getY().data());
        ssTitle.str("");
        ssTitle << "Phase" << i << " fsig = " << freq << "Hz";
        grPhase->SetName(ssTitle.str().c_str()); 
        grPhase->SetTitle(ssTitle.str().c_str()); 
        grPhase->GetXaxis()->SetLimits(1e4, 1e8);
        grPhase->GetHistogram()->SetMinimum(-2 * M_PI);
        grPhase->GetHistogram()->SetMaximum(2 * M_PI);
        c1->cd(2);
        grPhase->Draw("APL");

        grTrans = new TGraph(transient.getSize(), transient.getX().data(), transient.getY().data());
        ssTitle.str("");
        ssTitle << "Transient" << i << " fsig = " << freq << "Hz";
        grTrans->SetName(ssTitle.str().c_str()); 
        grTrans->SetTitle(ssTitle.str().c_str());
        grTrans->GetHistogram()->SetMinimum(-0.1);
        grTrans->GetHistogram()->SetMaximum(0.1);
        c1->cd(3);
        grTrans->Draw("APL");

        c1->Update();
        c1->SaveAs( (ssTitle.str() + ".png").c_str() );
        grMagn->Write();
        outTree->Fill();
    }

    // Cleanup
    grRMSvsFreq->Write();
    outTree->Write();
    outFile->Close();
    delete outFile;
    delete inFile;

    return 0;
}