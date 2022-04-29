#ifndef LINECHART_H
#define LINECHART_H

#include <QObject>
#include <QWidget>
#include <QList>
#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>
#include <QtMath>

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

struct Vector2D : public QPointF
{
    Vector2D(double x, double y) : QPointF(x, y)
    {
    }

    Vector2D(QPointF p) : QPointF(p)
    {
    }

    /// 向量长度
    double length()
    {
        return sqrt(x() * x() + y() * y());
    }

    /// 转单位向量
    Vector2D normalize()
    {
        double len = length();
        double inv;
        if (len < 1e-4)
            inv = 0;
        else
            inv = 1 / length();
        return Vector2D(x() * inv, y() * inv);
    }

    /// 向量相加
    Vector2D operator+ (Vector2D v)
    {
        return Vector2D(x() + v.x(), y() + v.y());
    }

    /// 向量翻倍
    Vector2D operator* (double f)
    {
        return Vector2D(x() * f, y() * f);
    }

    /// 内积
    double dot(Vector2D v)
    {
        return x() * v.x() + y() * v.y();
    }

    /// 两个向量夹角
    double angle(Vector2D v)
    {
        return acos(dot(v) / (length() * v.length())) * 180 / M_PI;
    }
};

class LineChart : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int display_x_min READ getDisplayXMin WRITE setDisplayXMin)
    Q_PROPERTY(int display_x_max READ getDisplayXMax WRITE setDisplayXMax)
    Q_PROPERTY(int display_y_min READ getDisplayYMin WRITE setDisplayYMin)
    Q_PROPERTY(int display_y_max READ getDisplayYMax WRITE setDisplayYMax)
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
    void setDisplayXMin(int v);
    int getDisplayXMin() const;
    void setDisplayXMax(int v);
    int getDisplayXMax() const;
    void setDisplayYMin(int v);
    int getDisplayYMin() const;
    void setDisplayYMax(int v);
    int getDisplayYMax() const;

    void saveRange();
    void startRangeAnimation();
    QPropertyAnimation* startAnimation(const QByteArray &property, int start, int end, int duration = 300, QEasingCurve curve = QEasingCurve::OutQuad);

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
    int pointLineType = 3;                  // 连线类型：1直线，2二次贝塞尔曲线，3三次贝塞尔曲线（更精确但吃性能）
    int pointValueType = 2;                 // 数值显示位置：0无，1强制上方，2自动附近，3自动省略
    int pointDotType = 1;                   // 圆点类型：0无，1空心圆，2实心圆，3小方块
    int pointDotRadius = 2;                 // 圆点半径

    // 交互数据
    bool pressing = false;
    QPoint pressPos;
    bool hovering = false;
    QPoint hoverPos;
    int nearDis = 8;                        // 四周这些距离内算是“附近”

    // 交互效果
    bool enableAnimation = true;
    bool showCrossOnPressing = true;        // 按下显示十字对准线
    QColor hightlightColor = Qt::red;       // 高亮颜色

    // 动画效果
    int _savedXMin, _savedXMax;
    int _savedYMin, _savedYMax;
};

#endif // LINECHART_H
