#include "linechart.h"
#include <QDebug>

LineChart::LineChart(QWidget *parent) : QWidget(parent)
{
    setMouseTracking(true);
}

int LineChart::lineCount() const
{
    return datas.size();
}

void LineChart::addData(ChartData data)
{
    saveRange();
    // 检查数据有效性
    if (!data.points.empty())
    {
        QPoint first = data.points.first();
        if (data.xMin == data.xMax)
        {
            data.xMin = data.xMax = first.x();
            for (QPoint p: data.points)
            {
                if (data.xMin > p.x())
                    data.xMin = p.x();
                else if (data.xMax < p.x())
                    data.xMax = p.x();
            }
        }
        if (data.yMin == data.yMax)
        {
            data.yMin = data.yMax = first.y();
            for (QPoint p: data.points)
            {
                if (data.yMin > p.y())
                    data.yMin = p.y();
                else if (data.yMax < p.y())
                    data.yMax = p.y();
            }
        }
    }
    Q_ASSERT(data.xLabels.empty() || data.xLabels.size() == data.points.size());

    // 新增的数据对当前视图的影响
    if (datas.empty()) // 第一次传入数据
    {
        displayXMin = data.xMin;
        displayXMax = data.xMax;
        displayYMin = data.yMin;
        displayYMax = data.yMax;
    }
    else
    {
        // 第二次及之后传入数据
        displayXMin = qMin(displayXMin, data.xMin);
        displayXMax = qMax(displayXMax, data.xMax);
        displayYMin = qMin(displayYMin, data.yMin);
        displayYMax = qMax(displayYMax, data.yMax);
    }

    // 新的label集合插入到现有X轴label中（多条数据融合）
    int index = 0;
    for (int i = 0; i < data.xLabels.size(); i++)
    {
        int x = data.points.at(i).x();
        const QString& label = data.xLabels.at(i);
        while (index < xLabels.size() && xLabelPoss.at(index) < x)
            index++;
        if (index >= xLabels.size()) // x超出范围了
        {
            xLabels.append(label);
            xLabelPoss.append(x);
        }
        else if (xLabelPoss.at(index) == x) // 一样的x，新的label，跳过
        {
            continue;
        }
        else if (xLabelPoss.at(index) > x)
        {
            xLabels.insert(index, label);
            xLabelPoss.insert(index, x);
        }
        else
        {
            qWarning() << data.points;
            qWarning() << data.xLabels;
            qWarning() << this->xLabels;
            qWarning() << this->xLabelPoss;
            qWarning() << index << x;
            Q_ASSERT(false);
        }
    }

    datas.append(data);

    startRangeAnimation();
}

void LineChart::addPoint(int index, int x, int y)
{
    saveRange();
    Q_ASSERT(index < datas.size());
    displayXMin = qMin(displayXMin, x);
    displayXMax = qMax(displayXMax, x);
    displayYMin = qMin(displayYMin, y);
    displayYMax = qMax(displayYMax, y);

    datas[index].points.append(QPoint(x, y));
    startRangeAnimation();
}

void LineChart::addPoint(int index, int x, int y, const QString &label)
{
    bool inserted = false;
    for (int i = xLabels.size() - 1; i >= 0; i--)
    {
        if (xLabelPoss.at(i) == x)
            break;
        if (xLabelPoss.at(i) < x)
        {
            xLabels.insert(i + 1, label);
            xLabelPoss.insert(i + 1, x);
            inserted = true;
            break;
        }
    }
    if (!inserted)
    {
        xLabels.insert(0, label);
        xLabelPoss.insert(0, x);
    }
    addPoint(index, x, y);
}

void LineChart::removeFirst(int index)
{
    Q_ASSERT(index <= datas.size());
    datas[index].points.removeFirst();
}

void LineChart::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // 画边界
    QRect contentRect(paddings.left(), paddings.top(),
                      width() - paddings.left() - paddings.width(),
                      height() - paddings.top() - paddings.height());
    painter.setPen(QPen(borderColor, 0.5));
    painter.drawRect(contentRect);

    if (datas.empty() || displayXMin >= displayXMax || displayYMin >= displayYMax)
        return ;

    // 画X轴数值
    QFontMetrics fm(painter.font());
    int lineSpacing = fm.lineSpacing();
    int lastRight = 0; // 上一次绘图的位置
    if (usePointXLabels && xLabels.size()) // 使用传入的label，可以是和数据对应的任意字符串
    {
        for (int i = 0; i < xLabels.size(); i++)
        {
            int val = xLabelPoss.at(i); // 数据x
            int x = contentRect.width() * (val - displayXMin) / (displayXMax - displayXMin); // 视图x
            int w = fm.horizontalAdvance(xLabels.at(i)); // 文字宽度
            int l = x - w / 2, r = x + w / 2; // 绘制的文字x范围
            if (!i || l > lastRight + labelSpacing || (i == xLabels.size() - 1 && (l = lastRight + labelSpacing)))
            {
                painter.drawText(QPoint(l + contentRect.left(),  contentRect.bottom() + lineSpacing), xLabels.at(i));
                lastRight = r;
            }
        }
    }
    else // 使用 xMin ~ xMax 的 int
    {
        bool highlighted = false;
        int maxTextWidth = qMax(fm.horizontalAdvance(QString::number(displayXMin)), fm.horizontalAdvance(QString::number(displayXMax)));
        int displayCount = qMax((contentRect.width() + labelSpacing) / (maxTextWidth + labelSpacing), 1); // 最多显示多少个标签
        int step = qMax((displayXMax - displayXMin + displayCount) / displayCount, 1);
        for (int i = displayXMin; i <= displayXMax; i += step)
        {
            int val = i;
            if (val > displayXMax - step)
                val = displayXMax; // 确保最大值一直显示
            int x = contentRect.width() * (val - displayXMin) / (displayXMax - displayXMin); // 视图x
            int w = fm.horizontalAdvance(QString::number(val)); // 文字宽度
            if (pressing && contentRect.contains(hoverPos) && (x + contentRect.left() >= hoverPos.x() - w && x + contentRect.left() <= hoverPos.x() + w))
            {
                x = hoverPos.x() - contentRect.left();
                val = (displayXMax - displayXMin) * x / contentRect.width();
                w = fm.horizontalAdvance(QString::number(val));
                highlighted = true;
            }
            int l = x - w / 2, r = x + w / 2;

            if (highlighted)
            {
                painter.save();
                painter.setPen(hightlightColor);
            }
            painter.drawText(QPoint(l + contentRect.left(),  contentRect.bottom() + lineSpacing), QString::number(val));
            if (highlighted)
            {
                painter.restore();
                highlighted = false;
            }
            lastRight = r;
        }
    }

    // 画Y轴数值
    for (int k = 0; k < datas.size() && k < 1; k++)
    {
        bool highlighted = false;
        int displayCount = qMax((contentRect.height() + labelSpacing) / (lineSpacing + labelSpacing), 1);
        int step = qMax((displayYMax - displayYMin + displayCount) / displayCount, 1);
        for (int i = displayYMin; i <= displayYMax; i += step)
        {
            if (i == displayYMin && i == 0 && displayXMin == 0) // X轴有0了，Y轴不重复显示
                continue;
            int val = i;
            if (val > displayYMax - step)
                val = displayYMax; // 确保最大值一直显示
            int y = contentRect.height() * (val - displayYMin) / (displayYMax - displayYMin);
            y = contentRect.bottom() - y;
            if (pressing && contentRect.contains(hoverPos) && (y >= hoverPos.y() - lineSpacing && y <= hoverPos.y() + lineSpacing))
            {
                y = hoverPos.y();
                val = (displayYMax - displayYMin) * (contentRect.bottom() - y) / contentRect.height() + displayYMin;
                highlighted = true;
            }
            int w = fm.horizontalAdvance(QString::number(val));

            if (highlighted)
            {
                painter.save();
                painter.setPen(hightlightColor);
            }
            if (k == 0) // 左边
            {
                painter.drawText(QPoint(contentRect.left() - labelSpacing - w, y + lineSpacing / 2), QString::number(val));
            }
            else // 右边
            {
                painter.drawText(QPoint(contentRect.right() + labelSpacing, y + lineSpacing / 2), QString::number(val));
            }
            if (highlighted)
            {
                painter.restore();
                highlighted = false;
            }
        }
    }

    // 画线条
    painter.save();
    for (int i = 0; i < datas.size(); i++)
    {
        const ChartData& line = datas.at(i);
        QPainterPath path;
        for (int j = 0; j < line.points.size(); j++)
        {
            const QPoint& pt = line.points.at(j);
            const QPoint contentPt(
                        contentRect.width() * (pt.x() - displayXMin) / (displayXMax - displayXMin),
                        contentRect.height() * (pt.y() - displayYMin) / (displayYMax - displayYMin)
                        );
            const QPoint dispt(
                        contentRect.left() + contentPt.x(),
                        contentRect.bottom() - contentPt.y()
                        );
            if (j == 0)
                path.moveTo(dispt);
            else
                path.lineTo(dispt);
        }
        painter.setPen(line.color);
        painter.drawPath(path);
    }
    painter.restore();

    // 画鼠标交互的线
    if (showCrossOnPressing && pressing && contentRect.contains(hoverPos))
    {
        painter.setPen(QPen(hightlightColor, 0.5, Qt::DashLine));
        painter.drawLine(contentRect.left(), hoverPos.y(), contentRect.right(), hoverPos.y());
        painter.drawLine(hoverPos.x(), contentRect.top(), hoverPos.x(), contentRect.bottom());
    }

    // 画鼠标交互的点
    if (hovering)
    {

    }


    // 画标题/图例


    // 画X轴文字

    // 画Y轴文字

}

void LineChart::enterEvent(QEvent *event)
{
    QWidget::enterEvent(event);

    hovering = true;
}

void LineChart::leaveEvent(QEvent *event)
{
    QWidget::leaveEvent(event);

    hovering = false;
}

void LineChart::mouseMoveEvent(QMouseEvent *event)
{
    QWidget::mouseMoveEvent(event);

    hoverPos = event->pos();

    if (pressing)
    {
        pressPos = hoverPos;
    }
    update();
}

void LineChart::mousePressEvent(QMouseEvent *event)
{
    QWidget::mousePressEvent(event);

    if (event->button() == Qt::LeftButton)
    {
        pressing = true;
    }
    update();
}

void LineChart::mouseReleaseEvent(QMouseEvent *event)
{
    QWidget::mouseReleaseEvent(event);

    if (event->button() == Qt::LeftButton)
    {
        pressing = false;
    }
    update();
}

void LineChart::setDisplayXMin(int v)
{
    this->displayXMin = v;
    update();
}

int LineChart::getDisplayXMin() const
{
    return this->displayXMin;
}

void LineChart::setDisplayXMax(int v)
{
    this->displayXMax = v;
    update();
}

int LineChart::getDisplayXMax() const
{
    return this->displayXMax;
}

void LineChart::setDisplayYMin(int v)
{
    this->displayYMin = v;
    update();
}

int LineChart::getDisplayYMin() const
{
    return this->displayYMin;
}

void LineChart::setDisplayYMax(int v)
{
    this->displayYMax = v;
    update();
}

int LineChart::getDisplayYMax() const
{
    return this->displayYMax;
}

void LineChart::saveRange()
{
    _savedXMin = displayXMin;
    _savedXMax = displayXMax;
    _savedYMin = displayYMin;
    _savedYMax = displayYMax;
}

void LineChart::startRangeAnimation()
{
    if (!enableAnimation)
    {
        update();
        return ;
    }
    if (_savedXMin != displayXMin)
        startAnimation("display_x_min", _savedXMin, displayXMin);
    if (_savedXMax != displayXMax)
        startAnimation("display_x_max", _savedXMax, displayXMax);
    if (_savedYMin != displayYMin)
        startAnimation("display_y_min", _savedYMin, displayYMin);
    if (_savedYMax != displayYMax)
        startAnimation("display_y_max", _savedYMax, displayYMax);
}

QPropertyAnimation *LineChart::startAnimation(const QByteArray &property, int start, int end, int duration, QEasingCurve curve)
{
    QPropertyAnimation* ani = new QPropertyAnimation(this, property);
    ani->setStartValue(start);
    ani->setEndValue(end);
    ani->setDuration(duration);
    ani->setEasingCurve(curve);
    connect(ani, SIGNAL(finished()), ani, SLOT(deleteLater()));
    ani->start();
    return ani;
}
