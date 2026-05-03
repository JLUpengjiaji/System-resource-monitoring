#ifndef TRENDCHART_H
#define TRENDCHART_H

#include <QWidget>
#include <QVector>
#include <QColor>
#include <QPen>
#include <QBrush>
#include <QPainter>
#include <QDateTime>

class TrendChart : public QWidget
{
    Q_OBJECT

public:
    enum ChartType
    {
        CPUChart,
        MemoryChart,
        NetworkUploadChart,
        NetworkDownloadChart
    };

    explicit TrendChart(ChartType type, QWidget *parent = nullptr);
    ~TrendChart();

    void addDataPoint(double value);
    void setMaxDataPoints(int maxPoints);
    void setTitle(const QString& title);
    void setUnit(const QString& unit);
    void setColor(const QColor& color);

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void drawBackground(QPainter& painter);
    void drawGrid(QPainter& painter);
    void drawData(QPainter& painter);
    void drawLabels(QPainter& painter);
    void drawCurrentValue(QPainter& painter);

    ChartType m_type;
    QVector<double> m_dataPoints;
    int m_maxDataPoints;
    QString m_title;
    QString m_unit;
    QColor m_lineColor;
    QColor m_backgroundColor;
    QColor m_gridColor;
    QColor m_textColor;
    QColor m_fillColor;
    double m_currentValue;
    double m_maxValue;
};

#endif // TRENDCHART_H
