#include "Mainwindow.h"
#include "ui_Mainwindow.h"
#include <QScreen>
#include <QMetaObject>
Mainwindow::Mainwindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Mainwindow)
    ,myCamera1(nullptr)
    ,CameraThread(nullptr)
{
    ui->setupUi(this);
    this->setWindowTitle("Camera");
    myCamera1 = new myCamera();
    CameraThread = new QThread(this);
    initWindow();
    setupCameraThread();
}

Mainwindow::~Mainwindow()
{
    delete ui;
}

void Mainwindow::initWindow()
{
    // 获取所有屏幕(适用于多个屏幕)
    QList<QScreen*> screens = QApplication::screens();
    if (screens.size() > 1) {
        QRect screenGeometry = screens[0]->geometry();
        move(screenGeometry.topLeft());
        showMaximized();
        //qDebug()<<size().width()<< size().height();
    }
    connect(ui->Preview,&QPushButton::clicked,this,&Mainwindow::on_Preview_clicked);
    connect(ui->Stop,&QPushButton::clicked,this,&Mainwindow::on_Stop_clicked);
    connect(ui->Capture,&QPushButton::clicked,this,&Mainwindow::on_Capture_clicked);

    containerWidget = new QWidget(this);
    containerWidget->setGeometry(ui->Camera->geometry());
    containerWidget->setObjectName("containerWidget");
    ui->Camera->setParent(containerWidget);
    ui->Camera->setGeometry(0, 0, ui->Camera->geometry().width(), ui->Camera->geometry().height());
    drawingWidget = new QWidget(containerWidget);
    drawingWidget->setGeometry(0, 0, ui->Camera->geometry().width(), ui->Camera->geometry().height());
    drawingWidget->raise();
    ui->Camera->lower();
    drawingWidget->setStyleSheet("background: transparent;");
    containerWidget->show();
}

void Mainwindow::setupCameraThread()
{
    myCamera1->moveToThread(CameraThread);
    connect(myCamera1, &myCamera::errorSignal, [](const QString& errorMsg) {
        qDebug()<< errorMsg;
    });
    connect(myCamera1, &myCamera::ImageReady,this, &Mainwindow::showImage);
    connect(CameraThread, &QThread::finished,myCamera1, &myCamera::deleteLater);
    connect(CameraThread, &QThread::finished,CameraThread, &QThread::deleteLater);
    CameraThread->start();
}

void Mainwindow::showImage(const QImage &showImage)
{
    QPixmap showPixmap = QPixmap::fromImage(showImage).scaled(QSize(2560,1400),Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
    ui->Camera->setPixmap(showPixmap);
}

void Mainwindow::on_Preview_clicked()
{
    qDebug()<<"prewviewing";
    QMetaObject::invokeMethod(myCamera1, "Preview", Qt::QueuedConnection);
}

void Mainwindow::on_Stop_clicked()
{
    qDebug()<<"stoping";
    QMetaObject::invokeMethod(myCamera1, "Stop", Qt::QueuedConnection);
}

void Mainwindow::on_Capture_clicked()
{
    //#1 捕获
    if(ui->Camera->pixmap()==NULL)
    {
        QMessageBox::warning(this,"warning","Cannot save pictures!");
        return;
    }
    QString savePathHead = QDir::rootPath()+"QtMVpicture/";
    QString curDate = QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss.zzz");
    QString format="bmp";//文件较大，较小用png
    QString savePath = savePathHead + curDate + "." + format;
    const QPixmap * picture_image=ui->Camera->pixmap();

    QPixmap watermarkedPixmap = *picture_image;
    // 创建 QPainter 来绘制文字
    QPainter painter(&watermarkedPixmap);
    // 设置字体
    QFont font("Arial", 16, QFont::Bold);
    painter.setFont(font);
    // 设置文字颜色（白色带黑色边框更清晰）
    painter.setPen(Qt::white);
    // 设置文字背景（可选，增加可读性）
    painter.setBrush(QColor(0, 0, 0, 128));  // 半透明黑色背景
    // 计算文字位置（右下角），绘制背景矩形，内容
    int padding = 10;
    QFontMetrics fm(font);
    int textWidth = fm.horizontalAdvance(curDate);
//    int textHeight = fm.height();
//    painter.drawRect(watermarkedPixmap.width() - textWidth - padding * 2,
//                     watermarkedPixmap.height() - textHeight - padding,
//                     textWidth + padding * 2,
//                     textHeight + padding);
    painter.drawText(watermarkedPixmap.width() - textWidth - padding,
                     watermarkedPixmap.height() - padding,
                     curDate);

    painter.end();

    if(watermarkedPixmap.save(savePath))
    {
        QMessageBox msgBox;
        msgBox.setText("save to "+savePath);
        msgBox.exec();
    }
    else
    {
        QErrorMessage eromsgBox;
        eromsgBox.showMessage("Failed to save picture");
    }
}

