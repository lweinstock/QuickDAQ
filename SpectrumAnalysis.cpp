#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#include <TFile.h>
#include <TTree.h>
#include <TGraph.h>
#include <TAxis.h>
#include <TCanvas.h>
#include <TH1.h>

#include <fftw3.h>

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
    TGraph* grSpec;
    TCanvas* c1 = new TCanvas();
    c1->SetLogx();
    c1->SetLogy();
    TTree* outTree = new TTree("outTree", "spectrumData");
    vector<double> freqFFT, magnFFT;
    outTree->Branch("freqFFT", &freqFFT);
    outTree->Branch("magnFFT", &magnFFT);

    // Read data in
    unsigned n = inTree->GetEntries();
    cout << "Found " << n << " entries" << endl;

    for (unsigned i = 0; i < n; i++)
    {
        inTree->GetEntry(i);
        cout << i << ": f=" << freq << ", vpp=" << vpp << endl;

        unsigned nPts = pTime->size();
        unsigned nFFT = nPts/2 + 1;
        double timeStep = abs(pTime->at(0) - pTime->at(1));
        fftw_complex* fft = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * nFFT);
        fftw_plan plan = fftw_plan_dft_r2c_1d(nPts, pVolt->data(), fft, FFTW_ESTIMATE);
        fftw_execute(plan);

        freqFFT.clear();
        magnFFT.clear();
        for (int j = 0; j < nFFT; j++) 
        {
            double f = j/(timeStep * nPts);
            double m = sqrt( fft[j][0]*fft[j][0] + fft[j][0]*fft[j][0] )/nPts;
            freqFFT.push_back(f);
            magnFFT.push_back(m);
        }

        grSpec = new TGraph(freqFFT.size(), freqFFT.data(), magnFFT.data());
        stringstream ssTitle;
        ssTitle << "Spectrum" << i << " fsig = " << freq << "Hz";
        grSpec->SetName(ssTitle.str().c_str()); 
        grSpec->SetTitle(ssTitle.str().c_str()); 
        grSpec->GetXaxis()->SetLimits(1e4, 1e8);
        //grSpec->GetYaxis()->SetLimits(1e-6, 1e-1);
        grSpec->GetHistogram()->SetMinimum(1e-6);
        grSpec->GetHistogram()->SetMaximum(1);
        grSpec->Draw("APL");
        c1->Update();
        c1->SaveAs( (ssTitle.str() + ".png").c_str() );
        grSpec->Write();
        outTree->Fill();

        // FFT cleanup
        fftw_destroy_plan(plan);
        fftw_free(fft);
    }

    // Cleanup
    outTree->Write();
    outFile->Close();
    delete outFile;
    delete inFile;

    return 0;
}