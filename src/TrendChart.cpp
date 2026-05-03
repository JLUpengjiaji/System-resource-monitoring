#include "TrendChart.h"
#include <QPainterPath>
#include <QLinearGradient>
#include <QFontMetrics>
#include <QDateTime>

TrendChart::TrendChart(ChartType type, QWidget *parent)
    : QWidget(parent)
    , m_type(type)
    , m_maxDataPoints(60)
    , m_currentValue(0.0)
    , m_maxValue(100.0)
{
    setAttribute(Qt::WA_StyledBackground, true);
    setMinimumWidth(200);
    setMinimumHeight(120);

    switch (m_type) {
    case CPUChart:
        m_title = tr("CPU");
        m_unit = tr("%");
        m_lineColor = QColor(255, 99, 71);
        m_fillColor = QColor(255, 99, 71, 50);
        m_maxValue = 100.0;
        break;
    case MemoryChart:
        m_title = tr("Memory");
        m_unit = tr("%");
        m_lineColor = QColor(54, 162, 235);
        m_fillColor = QColor(54, 162, 235, 50);
        m_maxValue = 100.0;
        break;
    case NetworkUploadChart:
        m_title = tr("Upload");
        m_unit = tr("KB/s");
        m_lineColor = QColor(75, 192, 192);
        m_fillColor = QColor(75, 192, 192, 50);
        m_maxValue = 1000.0;
        break;
    case NetworkDownloadChart:
        m_title = tr("Download");
        m_unit = tr("KB/s");
        m_lineColor = QColor(153, 102, 255);
        m_fillColor = QColor(153, 102, 255, 50);
        m_maxValue = 1000.0;
        break;
    }

    m_backgroundColor = QColor(30, 30, 30);
    m_gridColor = QColor(80, 80, 80);
    m_textColor = QColor(220, 220, 220);

    m_dataPoints.fill(0.0, m_maxDataPoints);
}

TrendChart::~TrendChart()
{
}

void TrendChart::addDataPoint(double value)
{
    m_currentValue = value;

    m_dataPoints.push_back(value);
    while (m_dataPoints.size() > m_maxDataPoints) {
        m_dataPoints.pop_front();
    }

    if (m_type == NetworkUploadChart || m_type == NetworkDownloadChart) {
        double maxValue = 0;
        for (double v : m_dataPoints) {
            if (v > maxValue) {
                maxValue = v;
            }
        }
        if (maxValue > m_maxValue * 0.9) {
            m_maxValue = maxValue * 1.5;
        } else if (maxValue < m_maxValue * 0.3 && m_maxValue > 100) {
            m_maxValue = qMax(100.0, maxValue * 2.0);
        }
    }

    update();
}

void TrendChart::setMaxDataPoints(int maxPoints)
{
    m_maxDataPoints = maxPoints;
    while (m_dataPoints.size() > m_maxDataPoints) {
        m_dataPoints.pop_front();
    }
}

void TrendChart::setTitle(const QString& title)
{
    m_title = title;
    update();
}

void TrendChart::setUnit(const QString& unit)
{
    m_unit = unit;
    update();
}

void TrendChart::setColor(const QColor& color)
{
    m_lineColor = color;
    m_fillColor = QColor(color.red(), color.green(), color.blue(), 50);
    update();
}

QSize TrendChart::minimumSizeHint() const
{
    return QSize(150, 100);
}

QSize TrendChart::sizeHint() const
{
    return QSize(250, 150);
}

void TrendChart::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    drawBackground(painter);
    drawGrid(painter);
    drawData(painter);
    drawLabels(painter);
    drawCurrentValue(painter);
}

void TrendChart::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    update();
}

void TrendChart::drawBackground(QPainter& painter)
{
    painter.fillRect(rect(), m_backgroundColor);

    QPen borderPen(m_gridColor, 1);
    painter.setPen(borderPen);
    painter.drawRect(rect().adjusted(0, 0, -1, -1));
}

void TrendChart::drawGrid(QPainter& painter)
{
    int paddingLeft = 40;
    int paddingRight = 10;
    int paddingTop = 25;
    int paddingBottom = 15;

    int chartWidth = width() - paddingLeft - paddingRight;
    int chartHeight = height() - paddingTop - paddingBottom;

    QPen gridPen(m_gridColor, 1, Qt::DashLine);
    painter.setPen(gridPen);

    int gridLines = 4;
    for (int i = 0; i <= gridLines; i++) {
        int y = paddingTop + (chartHeight * i) / gridLines;
        painter.drawLine(paddingLeft, y, paddingLeft + chartWidth, y);

        double value = m_maxValue - (m_maxValue * i) / gridLines;
        QString label = QString::number(value, 'f', 0);

        QFont font = painter.font();
        font.setPointSize(8);
        painter.setFont(font);

        QFontMetrics fm(font);
        int textWidth = fm.horizontalAdvance(label);
        int textHeight = fm.height();

        painter.setPen(m_textColor);
        painter.drawText(paddingLeft - textWidth - 5, y + textHeight / 4, label);
        painter.setPen(gridPen);
    }

    int timeLines = 6;
    for (int i = 0; i <= timeLines; i++) {
        int x = paddingLeft + (chartWidth * i) / timeLines;
        painter.drawLine(x, paddingTop, x, paddingTop + chartHeight);

        int secondsAgo = 60 - (60 * i) / timeLines;
        QString label;
        if (secondsAgo == 60) {
            label = tr("60s");
        } else if (secondsAgo == 0) {
            label = tr("Now");
        } else {
            label = QString::number(secondsAgo) + tr("s");
        }

        QFont font = painter.font();
        font.setPointSize(8);
        painter.setFont(font);

        QFontMetrics fm(font);
        int textWidth = fm.horizontalAdvance(label);

        painter.setPen(m_textColor);
        painter.drawText(x - textWidth / 2, height() - 5, label);
        painter.setPen(gridPen);
    }
}

void TrendChart::drawData(QPainter& painter)
{
    if (m_dataPoints.isEmpty()) {
        return;
    }

    int paddingLeft = 40;
    int paddingRight = 10;
    int paddingTop = 25;
    int paddingBottom = 15;

    int chartWidth = width() - paddingLeft - paddingRight;
    int chartHeight = height() - paddingTop - paddingBottom;

    QPainterPath path;
    QPainterPath fillPath;

    int dataCount = m_dataPoints.size();
    if (dataCount == 0) {
        return;
    }

    double stepX = static_cast<double>(chartWidth) / (m_maxDataPoints - 1);

    for (int i = 0; i < dataCount; i++) {
        double value = m_dataPoints[i];
        double normalizedValue = qMin(value / m_maxValue, 1.0);
        int x = paddingLeft + static_cast<int>(i * stepX);
        int y = paddingTop + chartHeight - static_cast<int>(normalizedValue * chartHeight);

        if (i == 0) {
            path.moveTo(x, y);
            fillPath.moveTo(x, paddingTop + chartHeight);
            fillPath.lineTo(x, y);
        } else {
            path.lineTo(x, y);
            fillPath.lineTo(x, y);
        }
    }

    if (dataCount > 0) {
        int lastX = paddingLeft + static_cast<int>((dataCount - 1) * stepX);
        fillPath.lineTo(lastX, paddingTop + chartHeight);
        fillPath.closeSubpath();
    }

    QLinearGradient gradient(0, paddingTop, 0, paddingTop + chartHeight);
    gradient.setColorAt(0, m_fillColor);
    gradient.setColorAt(1, QColor(m_fillColor.red(), m_fillColor.green(), m_fillColor.blue(), 0));

    painter.setBrush(gradient);
    painter.setPen(Qt::NoPen);
    painter.drawPath(fillPath);

    QPen linePen(m_lineColor, 2);
    painter.setPen(linePen);
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(path);

    for (int i = 0; i < dataCount; i++) {
        double value = m_dataPoints[i];
        double normalizedValue = qMin(value / m_maxValue, 1.0);
        int x = paddingLeft + static_cast<int>(i * stepX);
        int y = paddingTop + chartHeight - static_cast<int>(normalizedValue * chartHeight);

        if (i == dataCount - 1) {
            painter.setBrush(m_lineColor);
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(QPoint(x, y), 4, 4);
        }
    }
}

void TrendChart::drawLabels(QPainter& painter)
{
    QFont titleFont = painter.font();
    titleFont.setBold(true);
    titleFont.setPointSize(10);
    painter.setFont(titleFont);

    painter.setPen(m_textColor);
    painter.drawText(5, 5, width() - 10, 20, Qt::AlignLeft | Qt::AlignVCenter, m_title);
}

void TrendChart::drawCurrentValue(QPainter& painter)
{
    QString valueText;
    if (m_type == NetworkUploadChart || m_type == NetworkDownloadChart) {
        if (m_currentValue >= 1000) {
            valueText = QString::number(m_currentValue / 1000.0, 'f', 1) + tr(" MB/s");
        } else {
            valueText = QString::number(m_currentValue, 'f', 1) + tr(" KB/s");
        }
    } else {
        valueText = QString::number(m_currentValue, 'f', 1) + m_unit;
    }

    QFont valueFont = painter.font();
    valueFont.setBold(true);
    valueFont.setPointSize(12);
    painter.setFont(valueFont);

    QFontMetrics fm(valueFont);
    int textWidth = fm.horizontalAdvance(valueText);
    int textHeight = fm.height();

    QRect valueRect(width() - textWidth - 10, 5, textWidth, textHeight);
    painter.setPen(m_lineColor);
    painter.drawText(valueRect, Qt::AlignRight | Qt::AlignVCenter, valueText);
}
