#ifndef MY_WVFM_HH
#define MY_WVFM_HH

#include <vector>
#include <functional>
#include <fftw3.h>

class waveform
{
public:
    waveform() {};
    waveform(double* xData, double* yData, size_t len);
    waveform(std::vector<double> xData, std::vector<double> yData);
    ~waveform() {};

    void addPoint(double x, double y);
    void insertPoint(unsigned idx, double x, double y);

    double getX(unsigned i) { return m_xVal.at(i); } 
    double getY(unsigned i) { return m_yVal.at(i); } 
    std::vector<double> getX() { return m_xVal; }
    std::vector<double> getY() { return m_yVal; }
    size_t getSize() { return m_xVal.size(); }
    void clear();

    double getMean(unsigned sta, unsigned sto);
    double getMean() { return this->getMean(0, this->getSize() - 1); }
    double getRMS(unsigned sta, unsigned sto);
    double getRMS() { return this->getRMS(0, this->getSize() - 1); }
    double getSTD(unsigned sta, unsigned sto);
    double getSTD() { return this->getSTD(0, this->getSize() - 1); }

    void getFFT(waveform &magn, waveform &phase);

    // Static filter functions for use with applyFilter
    typedef std::function<double(double)> filterFunction;
    void applyFilter(filterFunction filter);
    static filterFunction gaus(double amplitude, double mean, double sigma);
    static filterFunction butterworthLowpass(double gain, double fc, unsigned n);
    static filterFunction butterworthHighpass(double gain, double fc, unsigned n);
    static filterFunction butterworthBandpass(double gain, double f1, double f2, unsigned n);

private:
    std::vector<double> m_xVal {};
    std::vector<double> m_yVal {};

    void checkStaSto(unsigned sta, unsigned sto);
};

#endif