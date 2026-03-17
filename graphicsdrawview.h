#ifndef GRAPHICSDRAWVIEW_H
#define GRAPHICSDRAWVIEW_H

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsLineItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsSimpleTextItem>
#include <QList>
#include <QPen>
#include <QBrush>
#include <QMouseEvent>

class GraphicsDrawView : public QGraphicsView
{
    Q_OBJECT
public:
    explicit GraphicsDrawView(QWidget *parent = nullptr);

    // 设置测量模式（开启后鼠标左键可绘制）
    void setMeasuring(bool enable);

    // 设置要显示的图像（自动缩放适应视图）
    void setImage(const QImage &image);

    // 清空所有已绘制的测量线
    void clearLines();

    // 手动添加一条测量线（可用于加载保存的数据）
    void addLine(const QPointF &p1, const QPointF &p2);

    // 将当前场景（图像+线条）渲染为 QImage，用于保存截图
    QImage renderToImage();

signals:
    // 当通过鼠标添加一条新线时发出
    void lineAdded(const QPointF &p1, const QPointF &p2);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;   // 支持滚轮缩放

private:
    void updateTempLine(const QPointF &pos);
    double calculateDistance(const QPointF &p1, const QPointF &p2) const;

    QGraphicsScene m_scene;
    QGraphicsPixmapItem *m_pixmapItem;          // 显示图像的图元
    QList<QGraphicsItem*> m_lineItems;          // 所有永久线条相关图元（方便清空）

    // 临时绘制用的图元（橡皮筋效果）
    QGraphicsLineItem *m_tempLineItem;
    QGraphicsEllipseItem *m_tempStartEllipse;
    QGraphicsEllipseItem *m_tempEndEllipse;
    QGraphicsSimpleTextItem *m_tempTextItem;

    bool m_measuring;   // 是否处于测量模式
    bool m_drawing;     // 是否正在绘制中
    QPointF m_startPoint; // 绘制起点（场景坐标，即图像像素坐标）
};

#endif // GRAPHICSDRAWVIEW_H
