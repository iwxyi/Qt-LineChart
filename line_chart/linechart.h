#ifndef LINECHART_H
#define LINECHART_H

#include <QObject>
#include <QWidget>
#include <QList>
#include <QPainter>
#include <QPainterPath>

struct ChartData
{
    int xMin = 0;
    int xMax = 0;
    int yMin = 0;
    int yMax = 0;
    QList<QPoint> points;
    QList<QString> xNames; // X显示的名字，可空，比如日期
    QColor color = Qt::black;
    QString title;
};

class LineChart : public QWidget
{
    Q_OBJECT
public:
    LineChart(QWidget *parent = nullptr);

signals:

public slots:
    void addData(ChartData data);
    void addPoint(int index, int x, int y);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QList<ChartData> datas;                 // 所有折线的数据

    QRect paddings = QRect(32, 32, 32, 32); // 四周留白(width=right,height=bottom)
    QColor borderColor = Qt::gray;          // 边界线颜色

    bool autoResize = true;                 // 自动调整大小
    int displayXMin = 0, displayXMax = 0;   // 显示的X轴范围
    int displayYMin = 0, displayYMax = 0;
    QList<QString> displayXStrs;            // 显示的文字（可能少于值数量）
    QList<QString> displayYStrs;
};

#endif // LINECHART_H
