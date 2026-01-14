#ifndef MYDISMEASURING_PEN_H
#define MYDISMEASURING_PEN_H

#include <QMainWindow>
#include <QPoint>
#include <QPainter>
#include <QLabel>
#include <QMouseEvent>
#include <QVector>
#include <QColor>
#include <cmath>
#include <QDebug>
#include "mvscamera.h"

struct MeasurementLine {
    QPoint startpoint;
    QPoint endpoint;
    double distance;

    MeasurementLine(const QPoint &s, const QPoint &e, double d)
        : startpoint(s), endpoint(e), distance(d) {}
    MeasurementLine() : distance(0) {}
};

class Mydismeasuring_pen : public MVSCamera
{
    Q_OBJECT

public:
    Mydismeasuring_pen(QWidget *parent = nullptr);
    ~Mydismeasuring_pen();

protected:
    void paintEvent(QPaintEvent *event)override;
    void mousePressEvent(QMouseEvent *event)override;
    void mouseMoveEvent(QMouseEvent *event)override;
    void mouseReleaseEvent(QMouseEvent *event)override;

private slots:
    void on_clear_lines_clicked();
    void on_dis_measure_clicked();

private:
    QVector<MeasurementLine> lines;  // 存储所有测量线
    QPoint currentStart;             // 当前测量起点
    QPoint currentEnd;               // 当前测量终点（鼠标移动时）
    bool isDrawing = false;          // 是否正在绘制（拖拽中）
    bool canMeasure=false;

    // 辅助函数
    double calculateDistance(const QPoint &p1, const QPoint &p2);
    void drawMeasurementLine(QPainter &painter, const MeasurementLine &line, int index);
    void drawCurrentLine(QPainter &painter, const QPoint &start, const QPoint &end);
    void drawPointMarker(QPainter &painter, const QPoint &point, const QColor &color);
};

#endif // MYDISMEASURING_PEN_H
