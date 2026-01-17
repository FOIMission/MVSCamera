#include "drawworker.h"
#include <QPainter>
#include <QDebug>

DrawWorker::DrawWorker()
    : m_isDrawing(false), m_frameDirty(0)
{
}

DrawWorker::~DrawWorker()
{
}

void DrawWorker::setVideoFrame(const QImage &frame)
{
    QMutexLocker locker(&m_mutex);
    if (!frame.isNull()) {
        m_videoFrame = frame.copy();
        m_frameDirty = 1;
    }
}

void DrawWorker::addLine(const QPoint &start, const QPoint &end, double distance)
{
    QMutexLocker locker(&m_mutex);
    m_lines.append(MeasurementLine(start, end, distance));
    m_frameDirty = 1;
}

void DrawWorker::updateCurrentLine(const QPoint &start, const QPoint &end)
{
    QMutexLocker locker(&m_mutex);
    m_currentStart = start;
    m_currentEnd = end;
    m_isDrawing = true;
    m_frameDirty = 1;
}

void DrawWorker::clearAllLines()
{
    QMutexLocker locker(&m_mutex);
    m_lines.clear();
    m_isDrawing = false;
    m_frameDirty = 1;
}

void DrawWorker::render()
{
    if (m_frameDirty.loadAcquire())
    {
        QImage result = renderImage();
        emit imageRendered(result);
        m_frameDirty.storeRelease(0);
    }
}

QImage DrawWorker::renderImage()
{
    QMutexLocker locker(&m_mutex);

    if (m_videoFrame.isNull()) {
        return QImage();
    }

    // 创建绘制图像
    QImage result = m_videoFrame.copy();
    QPainter painter(&result);
    painter.setRenderHint(QPainter::Antialiasing);

    // 绘制已完成的测量线
    for (int i = 0; i < m_lines.size(); i++)
    {
        const MeasurementLine &line = m_lines[i];
        QColor color = Qt::red;

        // 绘制连接线
        painter.setPen(QPen(color, 2));
        painter.drawLine(line.startpoint, line.endpoint);

        // 绘制端点
        painter.setPen(Qt::NoPen);
        painter.setBrush(color);
        painter.drawEllipse(line.startpoint, 4, 4);
        painter.drawEllipse(line.endpoint, 4, 4);

        // 显示距离文本
        QString text = QString("L%1: %2px").arg(i + 1).arg(line.distance, 0, 'f', 1);
        QPoint mid = (line.startpoint + line.endpoint) / 2;

        // 绘制文本背景
        painter.setPen(Qt::black);
        painter.setBrush(QColor(255, 255, 255, 230));
        QFontMetrics fm(painter.font());
        int textWidth = fm.horizontalAdvance(text) + 10;
        int textHeight = fm.height() + 6;
        painter.drawRect(mid.x() - textWidth/2, mid.y() - textHeight/2, textWidth, textHeight);

        // 绘制文本
        painter.drawText(mid.x() - textWidth/2 + 5, mid.y() + textHeight/2 - 5, text);
    }

    // 绘制当前正在拖拽的线
    if (m_isDrawing)
    {
        QPen pen(Qt::green);
        pen.setStyle(Qt::DashLine);
        pen.setWidth(2);
        painter.setPen(pen);
        painter.drawLine(m_currentStart, m_currentEnd);

        painter.setPen(Qt::NoPen);
        painter.setBrush(Qt::green);
        painter.drawEllipse(m_currentStart, 4, 4);
        painter.drawEllipse(m_currentEnd, 4, 4);

        double dist = QLineF(m_currentStart, m_currentEnd).length();
        QString text = QString("%1px").arg(dist, 0, 'f', 1);
        QPoint mid = (m_currentStart + m_currentEnd) / 2;

        painter.setPen(Qt::black);
        painter.setBrush(QColor(255, 255, 255, 200));
        QFontMetrics fm(painter.font());
        int textWidth = fm.horizontalAdvance(text) + 8;
        int textHeight = fm.height() + 4;
        painter.drawRect(mid.x() - textWidth/2, mid.y() - textHeight/2, textWidth, textHeight);
        painter.drawText(mid.x() - textWidth/2 + 4, mid.y() + textHeight/2 - 4, text);
    }

    painter.end();
    return result;
}
