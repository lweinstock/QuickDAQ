#include "waveform.hh"

#include <iostream>
#include <cmath>

using namespace std;

waveform::waveform(double* xData, double* yData, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        m_xVal.push_back(xData[i]);
        m_yVal.push_back(yData[i]);
    }
    return;
}

waveform::waveform(vector<double> xData, vector<double> yData)
{
    if ( xData.size() != yData.size() ) {
        
    }
    m_xVal = xData;
    m_yVal = yData;
    return;
}

void waveform::addPoint(double x, double y)
{
    m_xVal.push_back(x);
    m_yVal.push_back(y);
    return;
}

void waveform::insertPoint(unsigned idx, double x, double y)
{
    m_xVal.insert(m_xVal.begin() + idx, x);
    m_yVal.insert(m_yVal.begin() + idx, y);
    return;
}

void waveform::clear() 
{ 
    m_xVal.clear(); 
    m_yVal.clear(); 
    return;
}

double waveform::getMean(unsigned sta, unsigned sto)
{
    this->checkStaSto(sta, sto);
    double mean = .0;
    for (size_t i = sta; i < sto; i++)
        mean += m_yVal.at(i);
    return mean/(sto - sta);
}

double waveform::getRMS(unsigned sta, unsigned sto)
{
    this->checkStaSto(sta, sto);
    double rms = .0;
    for (size_t i = sta; i < sto; i++)
        rms += m_yVal.at(i) * m_yVal.at(i);
    rms /= (sto - sta);
    return sqrt(rms);
}

double waveform::getSTD(unsigned sta, unsigned sto)
{
    this->checkStaSto(sta, sto);
    double mean = .0, rms = .0;
    for (size_t i = sta; i < sto; i++) 
    {
        mean += m_yVal.at(i);
        rms += m_yVal.at(i) * m_yVal.at(i);
    }
    mean /=(sto - sta);
    rms /= (sto - sta);
    return (rms - mean*mean);
}

void waveform::getFFT(waveform &magn, waveform &phase)
{
    unsigned nPts = m_xVal.size();
    unsigned nFFT = nPts/2 + 1;
    double dx = abs(m_xVal.at(0) - m_xVal.at(1));
    fftw_complex* fft = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * nFFT);
    fftw_plan plan = fftw_plan_dft_r2c_1d(nPts, m_yVal.data(), fft, FFTW_ESTIMATE);
    fftw_execute(plan);

    magn.clear();
    phase.clear();
    for (int i = 0; i < nFFT; i++) 
    {
        double f = i/(dx * nPts);
        double m = sqrt( fft[i][0]*fft[i][0] + fft[i][1]*fft[i][1] )/nPts;
        double ph = atan(fft[i][1]/fft[i][0]);
        magn.addPoint(f, m);
        phase.addPoint(f, ph);
    }

    fftw_destroy_plan(plan);
    fftw_free(fft);
    return;
}

void waveform::applyFilter(filterFunction filter)
{
    unsigned nPts = m_xVal.size();
    unsigned nFFT = nPts/2 + 1;
    double dx = abs(m_xVal.at(0) - m_xVal.at(1));
    fftw_complex* fft = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * nFFT);
    fftw_plan plan = fftw_plan_dft_r2c_1d(nPts, m_yVal.data(), fft, FFTW_ESTIMATE);
    fftw_execute(plan);

    // Apply filter
    for (int i = 0; i < nFFT; i++) 
    {
        double f = i/(dx * nPts);
        fft[i][0] *= filter(f)/nPts;
        fft[i][1] *= filter(f)/nPts;
    }

    // Inverse transform
    plan = fftw_plan_dft_c2r_1d(nPts, fft, m_yVal.data(), FFTW_ESTIMATE);
    fftw_execute(plan);

    fftw_destroy_plan(plan);
    fftw_free(fft);   
    
    return;
}

/*
 *      S T A T I C   F I L T E R   M E T H O D S
 */

waveform::filterFunction waveform::gaus(double amplitude, double mean, double sigma) 
{
    return [amplitude, mean, sigma](double x) -> double {
        return amplitude * exp(-0.5 * (x - mean)*(x - mean)/(sigma * sigma));
    };
}

waveform::filterFunction waveform::butterworthLowpass(double gain, double fc, unsigned n)
{
    return [gain, fc, n](double x) -> double {
        return gain/sqrt(1 + pow((x/fc), 2*n));
    };
}

waveform::filterFunction waveform::butterworthHighpass(double gain, double fc, unsigned n)
{
    return [gain, fc, n](double x) -> double {
        return gain/sqrt(1 + pow((fc/x), 2*n));
    };
}

waveform::filterFunction waveform::butterworthBandpass(double gain, double f1, double f2,  unsigned n)
{
    return [gain, f1, f2, n](double x) -> double {
        double f0 = sqrt(f2*f1);
        double a = sqrt(f2/f1);
        return gain/sqrt(1 + pow( (x/f0 - f0/x)/a, 2*n));
    };
}

/*
 *      P R I V A T E   M E T H O D S
 */

void waveform::checkStaSto(unsigned sta, unsigned sto) {
    if ( (sta < this->getSize()) && (sto < this->getSize()) )
        return;
    std::cerr << "Invalid start and stop values: " << sta << " - " << sto << std::endl;
    abort();
    return;
}

