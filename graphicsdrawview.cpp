#include "graphicsdrawview.h"
#include <QWheelEvent>
#include <QFontMetrics>
#include <QDebug>

GraphicsDrawView::GraphicsDrawView(QWidget *parent)
    : QGraphicsView(parent)
    , m_pixmapItem(nullptr)
    , m_tempLineItem(nullptr)
    , m_tempStartEllipse(nullptr)
    , m_tempEndEllipse(nullptr)
    , m_tempTextItem(nullptr)
    , m_measuring(false)
    , m_drawing(false)
{
    setScene(&m_scene);
    setRenderHint(QPainter::Antialiasing);      // 抗锯齿
    setMouseTracking(true);                       // 追踪鼠标移动
    setBackgroundBrush(Qt::black);                // 背景黑色
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse); // 缩放锚点
    setResizeAnchor(QGraphicsView::AnchorUnderMouse);
}

void GraphicsDrawView::setMeasuring(bool enable)
{
    m_measuring = enable;
    if (!enable && m_drawing) {
        // 退出测量模式时取消正在绘制的临时线
        m_drawing = false;
        delete m_tempLineItem;       m_tempLineItem = nullptr;
        delete m_tempStartEllipse;   m_tempStartEllipse = nullptr;
        delete m_tempEndEllipse;     m_tempEndEllipse = nullptr;
        delete m_tempTextItem;       m_tempTextItem = nullptr;
    }
}

void GraphicsDrawView::setImage(const QImage &image)
{
    if (image.isNull())
        return;

    // 移除旧的图像图元
    if (m_pixmapItem) {
        m_scene.removeItem(m_pixmapItem);
        delete m_pixmapItem;
    }

    QPixmap pixmap = QPixmap::fromImage(image);
    m_pixmapItem = m_scene.addPixmap(pixmap);
    m_pixmapItem->setZValue(-1);      // 置于底层

    // 设置场景大小为图像实际像素尺寸
    m_scene.setSceneRect(0, 0, image.width(), image.height());

    // 自动缩放视图以完整显示图像
    fitInView(m_scene.sceneRect(), Qt::KeepAspectRatio);
}

void GraphicsDrawView::clearLines()
{
    for (auto item : m_lineItems) {
        m_scene.removeItem(item);
        delete item;
    }
    m_lineItems.clear();
}

void GraphicsDrawView::addLine(const QPointF &p1, const QPointF &p2)
{
    // 创建永久线条（红色、屏幕固定宽度）
    QPen pen(Qt::red);
    pen.setCosmetic(true);   // 线条在屏幕上始终为 2 像素宽
    pen.setWidth(2);
    QGraphicsLineItem *lineItem = new QGraphicsLineItem(QLineF(p1, p2));
    lineItem->setPen(pen);
    m_scene.addItem(lineItem);
    m_lineItems.append(lineItem);

    // 绘制端点（红色圆点）
    QBrush brush(Qt::red);
    QPen noPen(Qt::NoPen);
    QGraphicsEllipseItem *startEllipse = m_scene.addEllipse(-3, -3, 6, 6, noPen, brush);
    startEllipse->setPos(p1);
    m_lineItems.append(startEllipse);
    QGraphicsEllipseItem *endEllipse = m_scene.addEllipse(-3, -3, 6, 6, noPen, brush);
    endEllipse->setPos(p2);
    m_lineItems.append(endEllipse);

    // 显示距离文本（忽略视图变换，保持屏幕大小）
    double dist = calculateDistance(p1, p2);
    QString text = QString("%1px").arg(dist, 0, 'f', 1);
    QPointF mid = (p1 + p2) / 2;

    QGraphicsSimpleTextItem *textItem = new QGraphicsSimpleTextItem(text);
    textItem->setBrush(Qt::black);
    textItem->setFont(QFont("Arial", 10));
    textItem->setFlag(QGraphicsItem::ItemIgnoresTransformations, true); // 文本大小不受视图缩放影响
    QRectF rect = textItem->boundingRect();
    textItem->setPos(mid.x() - rect.width()/2, mid.y() - rect.height()/2);
    m_scene.addItem(textItem);
    m_lineItems.append(textItem);

    emit lineAdded(p1, p2);
}

QImage GraphicsDrawView::renderToImage()
{
    QImage image(m_scene.sceneRect().size().toSize(), QImage::Format_RGB32);
    image.fill(Qt::black);
    QPainter painter(&image);
    m_scene.render(&painter);
    return image;
}

void GraphicsDrawView::mousePressEvent(QMouseEvent *event)
{
    if (m_measuring && event->button() == Qt::LeftButton) {
        QPointF scenePos = mapToScene(event->pos());
        if (m_scene.sceneRect().contains(scenePos)) {
            m_drawing = true;
            m_startPoint = scenePos;

            // 临时线条（绿色虚线）
            m_tempLineItem = new QGraphicsLineItem(QLineF(m_startPoint, m_startPoint));
            QPen pen(Qt::green);
            pen.setCosmetic(true);
            pen.setWidth(2);
            pen.setStyle(Qt::DashLine);
            m_tempLineItem->setPen(pen);
            m_scene.addItem(m_tempLineItem);

            // 临时端点
            QBrush brush(Qt::green);
            QPen noPen(Qt::NoPen);
            m_tempStartEllipse = m_scene.addEllipse(-4, -4, 8, 8, noPen, brush);
            m_tempStartEllipse->setPos(m_startPoint);
            m_tempEndEllipse = m_scene.addEllipse(-4, -4, 8, 8, noPen, brush);
            m_tempEndEllipse->setPos(m_startPoint);

            // 临时距离文本
            m_tempTextItem = new QGraphicsSimpleTextItem("");
            m_tempTextItem->setBrush(Qt::black);
            m_tempTextItem->setFont(QFont("Arial", 10));
            m_tempTextItem->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
            m_scene.addItem(m_tempTextItem);

            updateTempLine(scenePos); // 初始化显示
        }
    }
    QGraphicsView::mousePressEvent(event);
}

void GraphicsDrawView::mouseMoveEvent(QMouseEvent *event)
{
    if (m_drawing) {
        QPointF scenePos = mapToScene(event->pos());
        updateTempLine(scenePos);
    }
    QGraphicsView::mouseMoveEvent(event);
}

void GraphicsDrawView::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_drawing && event->button() == Qt::LeftButton) {
        QPointF scenePos = mapToScene(event->pos());
        m_drawing = false;

        // 删除临时图元
        delete m_tempLineItem;       m_tempLineItem = nullptr;
        delete m_tempStartEllipse;   m_tempStartEllipse = nullptr;
        delete m_tempEndEllipse;     m_tempEndEllipse = nullptr;
        delete m_tempTextItem;       m_tempTextItem = nullptr;

        // 如果起点和终点不同且都在图像区域内，添加永久线
        if (m_startPoint != scenePos && m_scene.sceneRect().contains(scenePos)) {
            addLine(m_startPoint, scenePos);
        }
    }
    QGraphicsView::mouseReleaseEvent(event);
}

void GraphicsDrawView::wheelEvent(QWheelEvent *event)
{
    // 滚轮缩放
    qreal factor = 1.2;
    if (event->angleDelta().y() > 0)
        scale(factor, factor);
    else
        scale(1/factor, 1/factor);
}

void GraphicsDrawView::updateTempLine(const QPointF &pos)
{
    if (!m_tempLineItem) return;

    m_tempLineItem->setLine(QLineF(m_startPoint, pos));
    m_tempEndEllipse->setPos(pos);

    double dist = calculateDistance(m_startPoint, pos);
    QString text = QString("%1px").arg(dist, 0, 'f', 1);
    m_tempTextItem->setText(text);
    QPointF mid = (m_startPoint + pos) / 2;
    QRectF rect = m_tempTextItem->boundingRect();
    m_tempTextItem->setPos(mid.x() - rect.width()/2, mid.y() - rect.height()/2);
}

double GraphicsDrawView::calculateDistance(const QPointF &p1, const QPointF &p2) const
{
    return QLineF(p1, p2).length();
}
