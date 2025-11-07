#include "plot.hh"

#include <fstream>
#include <stdexcept>
#include <utility>

using namespace std;

wxPlot::wxPlot(wxWindow* parent, const vector<double>& xData, vector<double>& yData)
  : wxPanel(parent), m_xMargin(100), m_yMargin(50), m_labelOffs(3), 
    m_xTicks(10), m_yTicks(10), m_nMax(1000), 
    m_gridPen(wxColor(200, 200, 200), 1, wxPENSTYLE_DOT), 
    m_axisPen(*wxBLACK), m_xTitle(""), m_yTitle("")
{
    // Both data vector have to be same size
    assert(xData.size() == yData.size());
    size_t n = xData.size();
    m_data.reserve(n);  // reserve space to speed up process
    for (size_t i = 0; i < n; i++)
        m_data.emplace_back(xData.at(i), yData.at(i));

    this->AutoRange();
    this->Bind(wxEVT_PAINT, &wxPlot::OnPaint, this);
    return;
}


void wxPlot::SetRangeX(double xMin, double xMax)
{
    m_xMin = xMin;
    m_xMax = xMax;
    return;
}

void wxPlot::SetRangeY(double yMin, double yMax)
{
    m_yMin = yMin;
    m_yMax = yMax;
    return;
}

void wxPlot::AutoRange()
{
    m_xMin = numeric_limits<double>::max();
    m_xMax = numeric_limits<double>::lowest();
    m_yMin = numeric_limits<double>::max();
    m_yMax = numeric_limits<double>::lowest();

    // Find the max and min x and y values
    for (auto [x, y] : m_data)
    {
        if (x > m_xMax) m_xMax = x;
        if (x < m_xMin) m_xMin = x;
        if (y > m_yMax) m_yMax = y;
        if (y < m_yMin) m_yMin = y;
    }
    return;
}

void wxPlot::AutoRangeX()
{
    m_xMin = numeric_limits<double>::max();
    m_xMax = numeric_limits<double>::lowest();

    // Find the max and min x values
    for (auto [x, y] : m_data)
    {
        if (x > m_xMax) m_xMax = x;
        if (x < m_xMin) m_xMin = x;
    }
    return;
}

void wxPlot::AutoRangeY()
{
    m_yMin = numeric_limits<double>::max();
    m_yMax = numeric_limits<double>::lowest();

    // Find the max and min x values
    for (auto [x, y] : m_data)
    {
        if (y > m_yMax) m_yMax = y;
        if (y < m_yMin) m_yMin = y;
    }
    return;
}

void wxPlot::SetData(const vector<double> &x, const vector<double> &y)
{
    if (x.size() != y.size())
        throw std::out_of_range("X and Y vectors have to be of same size");

    size_t len = x.size();
    m_data.resize(len);
    for (size_t i = 0; i < len; i++)
        m_data[i] = make_pair(x[i], y[i]);

    return;
}

void wxPlot::AddData(double x, double y)
{
    if (m_data.size() == m_nMax)
        m_data.erase(m_data.begin());
    m_data.emplace_back(x, y);
    return;
}

void wxPlot::OnPaint(wxPaintEvent &ev)
{
    wxPaintDC dc(this);
    dc.SetBackground(*wxWHITE_BRUSH);
    dc.Clear();

    // Get panel size
    int width, height;
    this->GetClientSize(&width, &height);   // what about this->GetSize()?

    // Get bounds
    double xRange = (m_xMax - m_xMin);
    double yRange = (m_yMax - m_yMin);
    if ((xRange == 0) || (yRange == 0))
        return;
    
    // Define corners
    wxPoint topLeft(m_xMargin, m_yMargin);
    wxPoint botLeft(m_xMargin, height - m_yMargin);
    //wxPoint topRight(width - m_xMargin, m_yMargin);
    wxPoint botRight(width - m_xMargin, height - m_yMargin);

    this->DrawAxis(dc, AXIS_HORZ, botLeft, botRight, m_xMin, m_xMax, "z-position [inc]", m_xTicks);
    this->DrawAxis(dc, AXIS_VERT, botLeft, topLeft, m_yMin, m_yMax, "Force [N]", m_yTicks);
    this->DrawGrid(dc, botLeft, topLeft, botRight, m_xTicks, m_yTicks);
    
    // Draw data points
    int plotWidth = botRight.x - botLeft.x;
    int plotHeight = botLeft.y - topLeft.y;
    this->DrawData(dc, m_data, plotWidth, plotHeight);
    return;
}

void wxPlot::DrawAxis(wxDC &dc, AxisOrientation orient, wxPoint start, 
    wxPoint stop, double min, double max, wxString title, unsigned ticks)
{
    // Draw main axis
    dc.SetPen(m_axisPen);
    dc.DrawLine(start, stop);

    // Draw tick marks with labels
    dc.SetTextForeground(*wxBLACK);
    double plotSize = (orient == AXIS_HORZ) ? abs(stop.x - start.x) : abs(stop.y - start.y);
    double plotRange = max - min;
    int maxLabelWidth {0};  
    for (unsigned i = 0; i < ticks; i++)
    {
        double di = (i+1)/static_cast<double>(ticks);
        double val = min + di * plotRange;
        wxString label = wxString::Format("%.3e", val);

        int labelWidth, labelHeight;
        dc.GetTextExtent(label, &labelWidth, &labelHeight);

        // Find longest label to calc title offset for y-axis!
        if (labelWidth > maxLabelWidth)
            maxLabelWidth = labelWidth;

        wxPoint pt1, pt2, labelPos;
        if (orient == AXIS_HORZ)   // x-axis
        {
            pt1 = start + wxPoint(static_cast<int>(di*plotSize), 0);
            pt2 = pt1 - wxPoint(0, 5);
            labelPos = pt1 + wxPoint(-labelWidth/2, m_labelOffs);
        }
        else    // y-axis
        {
            pt1 = start - wxPoint(0, static_cast<int>(di*plotSize));
            pt2 = pt1 + wxPoint(5, 0);
            labelPos = pt1 - wxPoint(m_labelOffs + labelWidth, labelHeight/2);
        }
        dc.DrawLine(pt1, pt2);
        dc.DrawText(label, labelPos);
    }

    // Draw title
    wxPoint mid = (start + stop)/2;
    if (orient == AXIS_HORZ)
    {
        int titleWidth, titleHeight;
        dc.GetTextExtent(m_xTitle, &titleWidth, &titleHeight);
        int ptx = mid.x - titleWidth/2;
        int pty = mid.y + titleHeight + m_labelOffs;
        dc.DrawText(m_xTitle, ptx, pty);
    }
    else
    {
        int titleWidth, titleHeight;
        dc.GetTextExtent(m_yTitle, &titleWidth, &titleHeight);
        int ptx = mid.x - titleHeight - 2*m_labelOffs - maxLabelWidth;
        int pty = mid.y + titleWidth/2;
        dc.DrawRotatedText(m_yTitle, ptx, pty, 90);
    }
    return;
}


void wxPlot::DrawGrid(wxDC &dc, wxPoint botLeft, wxPoint topLeft, wxPoint botRight,
    unsigned xTicks, unsigned yTicks)
{
    dc.SetPen(m_gridPen);
    // x-grid
    for (unsigned i = 0; i < xTicks; i++)
    {
        double di = (i + 1)*(botRight.x - botLeft.x)/static_cast<double>(xTicks);
        dc.DrawLine(botLeft + wxPoint(di, 0), topLeft + wxPoint(di, 0));
    }
    // y-grid
    for (unsigned i = 0; i < yTicks; i++)
    {
        double di = (i + 1)*(botLeft.y - topLeft.y)/static_cast<double>(yTicks);
        dc.DrawLine(botLeft - wxPoint(0, di), botRight - wxPoint(0, di));
    }
    return;
}

void wxPlot::DrawData(wxDC &dc, const DataSet &data, int plotWidth, int plotHeight)
{
    dc.SetPen(*wxBLACK_PEN);
    for (auto [xval, yval] : data)
    {
        int xpt = m_xMargin + static_cast<int>( (xval - m_xMin)/(m_xMax - m_xMin) * plotWidth );
        int ypt = plotHeight + m_yMargin - static_cast<int>( (yval - m_yMin)/(m_yMax - m_yMin) * plotHeight );
        dc.DrawCircle(xpt, ypt, 3);
    }
    return;
}

void wxPlot::SaveData(std::string path)
{
    ofstream outFile(path);
    for (auto [x, y] : m_data)
        outFile << x << "," << y << endl;
    outFile.close();
    return;
}