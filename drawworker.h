#ifndef DRAWWORKER_H
#define DRAWWORKER_H

#include <QObject>
#include <QImage>
#include <QMutex>
#include <QAtomicInt>
#include <QPoint>
#include <QVector>
#include <QPainter>
#include <QLineF>
#include <QFontMetrics>

struct MeasurementLine {
    QPoint startpoint;
    QPoint endpoint;
    double distance;

    MeasurementLine(const QPoint &s, const QPoint &e, double d)
        : startpoint(s), endpoint(e), distance(d) {}
    MeasurementLine() : distance(0) {}
};

class DrawWorker : public QObject
{
    Q_OBJECT
public:
    DrawWorker();
    ~DrawWorker();

public slots:
    void setVideoFrame(const QImage &frame);
    void addLine(const QPoint &start, const QPoint &end, double distance);
    void updateCurrentLine(const QPoint &start, const QPoint &end);
    void clearAllLines();
    void render();

signals:
    void imageRendered(const QImage &result);

private:
    QMutex m_mutex;
    QImage m_videoFrame;
    QVector<MeasurementLine> m_lines;
    QPoint m_currentStart;
    QPoint m_currentEnd;
    bool m_isDrawing;
    QAtomicInt m_frameDirty;

    QImage renderImage();
};

#endif // DRAWWORKER_H
