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
    QFontMetrics fm(painter.font());
    int lineSpacing = fm.lineSpacing();
    const int labelSpacing = 2;
    int lastRight = 0; // 上一次绘图的位置
    if (usePointLabels && xLabels.size()) // 使用传入的label，可以是和数据对应的任意字符串
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
        int maxTextWidth = qMax(fm.horizontalAdvance(QString::number(displayXMin)), fm.horizontalAdvance(QString::number(displayXMax)));
        int displayCount = (contentRect.width() + labelSpacing) / (maxTextWidth + labelSpacing); // 最多显示多少个标签
        int step = int((displayXMax - displayXMin + displayCount) / displayCount);
        for (int i = displayXMin; i <= displayXMax; i += step)
        {
            int val = i;
            if (val > displayXMax - step)
                val = displayXMax; // 确保最大值一直显示
            int x = contentRect.width() * (val - displayXMin) / (displayXMax - displayXMin); // 视图x
            int w = fm.horizontalAdvance(QString::number(val)); // 文字宽度
            int l = x - w / 2, r = x + w / 2;

            painter.drawText(QPoint(l + contentRect.left(),  contentRect.bottom() + lineSpacing), QString::number(val));
            lastRight = r;
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
