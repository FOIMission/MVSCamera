#include "mydismeasuring_pen.h"
#include "ui_Mainwindow.h"

Mydismeasuring_pen::Mydismeasuring_pen(QWidget *parent)
    : Mainwindow(parent)
{
    connect(ui->clear_lines, &QPushButton::clicked, this, &Mydismeasuring_pen::on_clear_lines_clicked);
    connect(ui->dis_measure, &QPushButton::clicked, this, &Mydismeasuring_pen::on_dis_measure_clicked);
}

Mydismeasuring_pen::~Mydismeasuring_pen()
{
    lines.clear();
}

void Mydismeasuring_pen::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    for (int i = 0; i < lines.size(); i++)
    {
        drawMeasurementLine(painter, lines[i], i);
    }

    if (isDrawing)
    {
        drawCurrentLine(painter, currentStart, currentEnd);
    }
}

void Mydismeasuring_pen::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton&&canMeasure)
    {
        // 按下鼠标：开始拖拽绘制
        currentStart = event->pos();;
        currentEnd = event->pos();;
        isDrawing = true;
        update();  // 触发重绘
    }
}

void Mydismeasuring_pen::mouseMoveEvent(QMouseEvent *event)
{
    if (isDrawing)
    {
        // 拖拽过程中：更新终点位置
        currentEnd = event->pos();
        update();  // 触发重绘
    }
}

void Mydismeasuring_pen::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && isDrawing)
    {
        currentEnd = event->pos();;
        if (currentStart != currentEnd)
        {
            double dist = calculateDistance(currentStart, currentEnd);
            MeasurementLine line(currentStart, currentEnd, dist);
            lines.append(line);
        }
        isDrawing = false;
        update();
    }
}

// 计算距离
double Mydismeasuring_pen::calculateDistance(const QPoint &p1, const QPoint &p2)
{
    double dx = p2.x() - p1.x();
    double dy = p2.y() - p1.y();
    return std::sqrt(dx * dx + dy * dy);
}

// 绘制一条已完成的测量线
void Mydismeasuring_pen::drawMeasurementLine(QPainter &painter, const MeasurementLine &line, int index)
{
    QColor color = Qt::black;

    // 绘制连接线
    painter.setPen(QPen(color, 2));
    painter.drawLine(line.startpoint, line.endpoint);

    // 绘制端点标记 - 修复：使用fillRect或fillEllipse绘制彩色点
    painter.setPen(Qt::NoPen);  // 不要黑色边框
    painter.setBrush(color);    // 设置填充颜色
    painter.drawEllipse(line.startpoint, 4, 4);  // 绘制起点
    painter.drawEllipse(line.endpoint, 4, 4);    // 绘制终点

    // 显示距离文本
    QString text = QString("L%1: %2px").arg(index + 1).arg(line.distance, 0, 'f', 1);
    QPoint mid = (line.startpoint + line.endpoint) / 2;

    // 绘制文本背景
    painter.setPen(Qt::black);
    painter.setBrush(QColor(255, 255, 255, 230));  // 白色半透明背景
    QFontMetrics fm(painter.font());
    int textWidth = fm.horizontalAdvance(text) + 10;
    int textHeight = fm.height() + 6;
    painter.drawRect(mid.x() - textWidth/2, mid.y() - textHeight/2, textWidth, textHeight);

    // 绘制文本
    painter.drawText(mid.x() - textWidth/2 + 5, mid.y() + textHeight/2 - 5, text);
}

// 绘制当前正在拖拽的线
void Mydismeasuring_pen::drawCurrentLine(QPainter &painter, const QPoint &startpoint, const QPoint &endpoint)
{
    QPen pen(Qt::black);
    pen.setStyle(Qt::DashLine);
    painter.setPen(Qt::black);
    painter.drawLine(startpoint, endpoint);

    painter.setBrush(Qt::black);
    painter.drawEllipse(startpoint, 4, 4);

    painter.setBrush(Qt::black);
    painter.drawEllipse(endpoint, 4, 4);

    double dist = calculateDistance(startpoint, endpoint);
    QString text = QString("%1px").arg(dist, 0, 'f', 1);
    QPoint mid = (startpoint + endpoint) / 2;

    painter.setPen(Qt::black);
    painter.setBrush(QColor(255, 255, 255, 200));
    QFontMetrics fm(painter.font());
    int textWidth = fm.horizontalAdvance(text) + 8;
    int textHeight = fm.height() + 4;
    painter.drawRect(mid.x() - textWidth/2, mid.y() - textHeight/2, textWidth, textHeight);
    painter.drawText(mid.x() - textWidth/2 + 4, mid.y() + textHeight/2 - 4, text);
}

void Mydismeasuring_pen::on_dis_measure_clicked()
{
    if(!canMeasure)
    {
        ui->dis_measure->setText("isMeasuring");
        canMeasure=true;
    }
    else
    {
        ui->dis_measure->setText("dis_measure");
        canMeasure=false;
    }
}

void Mydismeasuring_pen::on_clear_lines_clicked()
{
    lines.clear();
    isDrawing=false;
    update();
}


