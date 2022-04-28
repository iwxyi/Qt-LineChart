#ifndef LINECHART_H
#define LINECHART_H

#include <QObject>
#include <QWidget>
#include <QList>
#include <QPainter>
#include <QPainterPath>

struct ChartData
{
    QString title;
    QColor color = Qt::black;
    int xMin = 0;
    int xMax = 0;
    int yMin = 0;
    int yMax = 0;
    QList<QPoint> points;
    QList<QString> xLabels; // X显示的名字，可空，比如日期
};

class LineChart : public QWidget
{
    Q_OBJECT
public:
    LineChart(QWidget *parent = nullptr);

    int lineCount() const;

signals:

public slots:
    void addData(ChartData data);
    void addPoint(int index, int x, int y);
    void addPoint(int index, int x, int y, const QString& label);
    void removeFirst(int index);

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    // 数据
    QList<ChartData> datas;                 // 所有折线的数据

    // 界面
    QRect paddings = QRect(32, 32, 32, 32); // 四周留白(width=right,height=bottom)
    QColor borderColor = Qt::gray;          // 边界线颜色
    int labelSpacing = 2;                   // 标签间距

    // 信息显示
    bool autoResize = true;                 // 自动调整大小
    int displayXMin = 0, displayXMax = 0;   // 显示的X轴范围
    int displayYMin = 0, displayYMax = 0;   // 显示的Y轴范围
    bool usePointXLabels = true;            // 优先使用点对应的label，还是相同间距的数值
    QList<QString> xLabels;                 // 显示的文字（可能少于值数量）
    QList<int> xLabelPoss;
    bool showPointValue = true;             // 显示每一个点的数值
    bool showPointDot = true;               // 显示每一个点的圆圈

    // 交互数据
    bool pressing = false;
    QPoint pressPos;
    bool hovering = false;
    QPoint hoverPos;

    // 交互效果
    bool showCrossOnPressing = true;        // 按下显示十字对准线
    QColor hightlightColor = Qt::red;       // 高亮颜色
};

#endif // LINECHART_H
