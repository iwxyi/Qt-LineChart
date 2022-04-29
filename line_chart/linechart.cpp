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
    if (datas.at(index).points.empty())
        return ;
    bool equal = (displayXMin == datas.at(index).points.first().x());
    datas[index].points.removeFirst();

    // 调整最小值
    if (equal)
    {
        // saveRange();
        int newXMin = displayXMax;
        for (int i = 0; i < datas.size(); i++)
            if (!datas.at(i).points.empty())
            {
                int x = datas.at(i).points.first().x();
                if (x < newXMin)
                    newXMin = x;
            }
        displayXMin = newXMin;
        update();
        // startRangeAnimation();
    }
}

void LineChart::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    /// 边界
    QRect contentRect(paddings.left(), paddings.top(),
                      width() - paddings.left() - paddings.width(),
                      height() - paddings.top() - paddings.height());
    painter.setPen(QPen(borderColor, 0.5));
    painter.drawRect(contentRect);

    if (datas.empty() || displayXMin >= displayXMax || displayYMin >= displayYMax)
        return ;

    QFontMetrics fm(painter.font());
    int lineSpacing = fm.height();
    QPoint accessNearestPos = hoverPos;
    int accessMinDis = 0x3f3f3f3f;

    /// 画线条与数值
    painter.save();
    QPainterPath contentPath;
    painter.setClipRect(contentRect);
    for (int i = 0; i < datas.size(); i++)
    {
        // 计算点要绘制的所有坐标
        QList<QPoint> displayPoints;
        const ChartData& line = datas.at(i);
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
            displayPoints.append(dispt);
        }

        // 连线
        if (pointLineType && displayPoints.size() > 1)
        {
            QPainterPath path;
            if (pointLineType == 1) // 直线
            {
                path.moveTo(displayPoints.first());
                for (int i = 1; i < displayPoints.size(); i++)
                    path.lineTo(displayPoints.at(i));
            }
            else if (pointLineType == 2) // 二次贝塞尔曲线
            {
                // TODO: 二次贝塞尔曲线
            }
            else if (pointLineType == 3) // 三次贝塞尔曲线
            {
                // 算法参考：https://juejin.cn/post/6844903477273952270
                // 源码参考：https://github.com/AlloyTeam/curvejs/blob/master/asset/smooth.html
                double rt = 0.2; // 平滑度
                QList<Vector2D> controlPoints;
                const auto& points = displayPoints;
                int count = points.size() - 2;
                for (int i = 0; i < count; i++)
                {
                    QPoint a = points.at(i), b = points.at(i+1), c = points.at(i+2);
                    Vector2D v1(a - b);
                    Vector2D v2(c - b);
                    double v1Len = v1.length(), v2Len = v2.length();
                    Vector2D centerV = (v1.normalize() + v2.normalize()).normalize();

                    Vector2D ncp1(centerV.y(), centerV.x() * - 1);
                    Vector2D ncp2(centerV.y() * -1, centerV.x());
                    if (ncp1.angle(v1) < 90)
                    {
                        Vector2D p1 = ncp1 * (v1Len * rt) + b;
                        Vector2D p2 = ncp2 * (v2Len * rt) + b;
                        controlPoints.append(p1);
                        controlPoints.append(p2);
                    }
                    else
                    {
                        Vector2D p1 = ncp1 * (v2Len * rt) + b;
                        Vector2D p2 = ncp2 * (v1Len * rt) + b;
                        controlPoints.append(p2);
                        controlPoints.append(p1);
                    }
                }

                path.moveTo(points.at(0));
                path.cubicTo(points.at(0), controlPoints.at(0), points.at(1));

                for (int i = 1; i < count; i++)
                {
                    path.moveTo(points.at(i));
                    path.cubicTo(controlPoints.at(i * 2 - 1), controlPoints.at(i * 2), QPointF(points.at(i+1)));
                }

                path.moveTo(points.at(count));
                path.cubicTo(controlPoints.last(), points.last(), points.last());
            }
            painter.setPen(line.color);
            painter.drawPath(path);
        }

        // 绘制点的小圆点
        if (pointDotType)
        {
            for (int i = 0; i < displayPoints.size(); i++)
            {
                const QPoint& pt = displayPoints.at(i);
                QRect pointRect(pt.x() - pointDotRadius, pt.y() - pointDotRadius, pointDotRadius * 2, pointDotRadius * 2);
                if (pointDotType == 1) // 空心圆
                {
                    painter.drawEllipse(pointRect);
                }
                else if (pointDotType == 2) // 实心圆
                {
                    QPainterPath path;
                    path.addEllipse(pointRect);
                    painter.fillPath(path, line.color);
                }
                else if (pointDotType == 3) // 小方块
                {
                    painter.fillRect(pointRect, line.color);
                }
            }
        }

        // 绘制数值
        if (pointValueType)
        {
            const QList<QPoint>& points = line.points;
            for (int i = 0; i < points.size(); i++)
            {
                QString text = QString::number(points.at(i).y());
                QPoint pos = displayPoints.at(i);
                int w = fm.horizontalAdvance(text);
                int x = pos.x() - w / 2;
                int y = pos.y() - pointDotRadius - fm.leading(); // 默认是强制正上方位置
                if (pointValueType == 2) // 所有，自动选取合适的
                {
                    if (i == 0 && i < points.size() - 1)
                    {
                        if (points.at(i + 1).y() > points.at(i).y()) // 显示在下方
                            y = pos.y() + lineSpacing + pointDotRadius;
                    }
                    else if (i > 0 && i < points.size() - 1)
                    {
                        int v = points.at(i).y();
                        int vl = points.at(i-1).y();
                        int vr = points.at(i+1).y();
                        if (vl > v && vr > v) // V型，显示在下面
                            y = pos.y() + lineSpacing + pointDotRadius;
                        else if (vl < v && vr > v) // 显示偏左
                            x -= w / 2;
                        else if (vl > v && vr < v) // 显示偏右
                            x += w / 2;
                        // TODO: 还可以根据两侧斜率来进一步优化
                    }
                    else if (i == points.size() - 1 && points.size() > 1)
                    {
                        if (points.at(i-1).y() > points.at(i).y()) // 显示在下方
                            y = pos.y() + lineSpacing + pointDotRadius;
                    }
                }
                else if (pointValueType == 3) // 选合适的进行显示
                {
                    // TODO: 判断密集程度
                    continue;
                }

                // 判断超出边界
                if (x - w / 2 < contentRect.left())
                    x = contentRect.left();
                else if (x + w > contentRect.right())
                    x = contentRect.right() - w;
                if (y - lineSpacing < contentRect.top())
                    y = pos.y() + lineSpacing + pointDotRadius;
                else if (y > contentRect.bottom())
                    y = pos.y() - pointDotRadius - fm.leading();

                // 绘制文字
                painter.drawText(QPoint(x, y), text);
            }

        }

        // 计算距离最近的点
        if (hovering && contentRect.contains(hoverPos))
        {
            for (int i = 0; i < displayPoints.size(); i++)
            {
                const QPoint& pt = displayPoints.at(i);
                if (qAbs(hoverPos.x() - pt.x()) > nearDis || qAbs(hoverPos.y() - pt.y()) > nearDis)
                    continue;
                int distance = (hoverPos - displayPoints.at(i)).manhattanLength();
                if (distance < accessMinDis)
                {
                    accessNearestPos = displayPoints.at(i);
                    accessMinDis = distance;
                }
            }
        }
    }
    painter.restore();

    if (accessMinDis != 0x3f3f3f3f)
    {
        painter.save();
        painter.setPen(hightlightColor);
        QRect pointRect(accessNearestPos.x() - pointDotRadius, accessNearestPos.y() - pointDotRadius, pointDotRadius * 2, pointDotRadius * 2);
        if (pointDotType == 0 || pointDotType == 1) // 空心圆
        {
            painter.drawEllipse(pointRect);
        }
        else if (pointDotType == 2) // 实心圆
        {
            QPainterPath path;
            path.addEllipse(pointRect);
            painter.fillPath(path, hightlightColor);
        }
        else if (pointDotType == 3) // 小方块
        {
            painter.fillRect(pointRect, hightlightColor);
        }
        painter.restore();
    }


    /// 画坐标轴
    // 画X轴数值
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
            if (hovering && contentRect.contains(accessNearestPos) && (x + contentRect.left() >= accessNearestPos.x() - w && x + contentRect.left() <= accessNearestPos.x() + w))
            {
                x = accessNearestPos.x() - contentRect.left();
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
            if (i == displayYMin && displayYMin == 0 && displayXMin == 0) // X轴有0了，Y轴不重复显示
                continue;
            int val = i;
            if (val > displayYMax - step)
            {
                if (val < displayYMax - step / 2)
                    i = displayYMax - step;
                else
                    val = displayYMax; // 确保最大值一直显示
            }
            int y = contentRect.height() * (val - displayYMin) / (displayYMax - displayYMin);
            y = contentRect.bottom() - y;
            if (hovering && contentRect.contains(accessNearestPos) && (y >= accessNearestPos.y() - lineSpacing && y <= accessNearestPos.y() + lineSpacing))
            {
                y = accessNearestPos.y();
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

    /// 交互
    // 画鼠标交互的线
    if (showCrossOnPressing && hovering && contentRect.contains(accessNearestPos))
    {
        painter.setPen(QPen(hightlightColor, 0.5, Qt::DashLine));
        painter.drawLine(contentRect.left(), accessNearestPos.y(), contentRect.right(), accessNearestPos.y());
        painter.drawLine(accessNearestPos.x(), contentRect.top(), accessNearestPos.x(), contentRect.bottom());
    }


    // 画标题/图例


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
    update();
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
