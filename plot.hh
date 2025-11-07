#ifndef WX_PLOT_HH
#define WX_PLOT_HH

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif
#include <wx/propgrid/propgrid.h>

#include <vector>

class wxPlot : public wxPanel 
{
public:
    wxPlot(wxWindow* parent, const std::vector<double>& xData, std::vector<double>& yData);

    void SetTicksX(unsigned n) { m_xTicks = n; }
    void SetTicksY(unsigned n) { m_yTicks = n; }

    void SetTitleX(wxString label) { m_xTitle = label; }
    void SetTitleY(wxString label) { m_yTitle = label; }

    void SetRangeX(double xMin, double xMax);
    void SetRangeY(double yMin, double yMax);
    void AutoRange();
    void AutoRangeX();
    void AutoRangeY();

    void SetData(const std::vector<double> &x, const std::vector<double> &y);
    void AddData(double x, double y);
    void ClearData() { m_data.clear(); }
    void SaveData(std::string path);

    void OnPaint(wxPaintEvent &ev);

private:

    enum AxisOrientation {AXIS_HORZ, AXIS_VERT};
    void DrawAxis(wxDC &dc, AxisOrientation orient, wxPoint start, wxPoint stop, 
        double min, double max, wxString title, unsigned ticks = 10);

    void DrawGrid(wxDC &dc, wxPoint botLeft, wxPoint topLeft, wxPoint botRight,
        unsigned xTicks = 10, unsigned yTicks = 10);

    typedef std::vector<std::pair<double, double>> DataSet;
    void DrawData(wxDC &dc, const DataSet &data, int plotWidth, int plotHeight);
        
    // x/y data containers -> maybe ill expand it to vector<Data> at some point...
    DataSet m_data;
    
    double m_xMin, m_xMax, m_yMin, m_yMax;
    int m_xMargin, m_yMargin, m_labelOffs;
    unsigned m_xTicks, m_yTicks, m_nMax;
    wxPen m_gridPen, m_axisPen;
    wxString m_xTitle, m_yTitle;
};

#endif