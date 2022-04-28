#include "linechart.h"
#include <QDebug>

LineChart::LineChart(QWidget *parent) : QWidget(parent)
{

}

void LineChart::addData(ChartData data)
{
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
qDebug() << data.xMin << data.xMax << data.yMin << data.yMax;
qDebug() << data.points;

    // 新增的数据对当前视图的影响
    if (datas.empty())
    {
        displayXMin = data.xMin;
        displayXMax = data.xMax;
        displayYMin = data.yMin;
        displayYMax = data.yMax;
    }
    else
    {
        displayXMin = qMin(displayXMin, data.xMin);
        displayXMax = qMax(displayXMax, data.xMax);
        displayYMin = qMin(displayYMin, data.yMin);
        displayYMax = qMax(displayYMax, data.yMax);
    }
    datas.append(data);

    // 仅第一次传入有效
    if (data.xNames.empty())
    {
        for (int i = 0; i < data.points.size(); i++)
            data.xNames.append(QString::number(data.points.at(i).x()));
    }

    update();
}

void LineChart::addPoint(int index, int x, int y)
{
    Q_ASSERT(index < datas.size());
    Q_UNUSED(x)
    Q_UNUSED(y)
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
    painter.setPen(borderColor);
    painter.drawRect(contentRect);

    if (datas.empty() || displayXMin >= displayXMax || displayYMin >= displayYMax)
        return ;

    // 画数值
    if (displayXStrs.size())
    {

    }

    if (displayXStrs.size())
    {

    }

    // 画线条
    painter.save();
    painter.setWindow(-paddings.left(), height() - paddings.top(), width(), -height());
    for (int i = 0; i < datas.size(); i++)
    {
        const ChartData& line = datas.at(i);
        QPainterPath path;
        for (int j = 0; j < line.points.size(); j++)
        {
            const QPoint& pt = line.points.at(j);
            const QPoint dispt(
                        contentRect.width() * (pt.x() - displayXMin) / (displayXMax - displayXMin),
                        contentRect.height() * (pt.y() - displayYMin) / (displayYMax - displayYMin)
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

    // 画鼠标交互的点


    // 画标题/图例


    // 画X轴文字

    // 画Y轴文字

}

void LineChart::mouseMoveEvent(QMouseEvent *event)
{
    QWidget::mouseMoveEvent(event);
}

void LineChart::mousePressEvent(QMouseEvent *event)
{
    QWidget::mousePressEvent(event);
}

void LineChart::mouseReleaseEvent(QMouseEvent *event)
{
    QWidget::mouseReleaseEvent(event);
}
