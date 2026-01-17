#include "mvscamera.h"
#include "ui_mvscamera.h"
#include "drawworker.h"
#include <QMouseEvent>

MVSCamera::MVSCamera(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MVSCamera)
    , m_drawWorker(nullptr)
    , m_drawThread(nullptr)
    , m_isMeasuring(false)
    , m_isDrawing(false)
{
    ui->setupUi(this);
    initWindow();

    // 初始化绘制系统
    m_drawWorker = new DrawWorker();
    m_drawThread = new QThread(this);
    m_drawWorker->moveToThread(m_drawThread);

    connect(m_drawThread, &QThread::finished, m_drawWorker, &DrawWorker::deleteLater);
    connect(this, &MVSCamera::requestRender, m_drawWorker, &DrawWorker::render);
    connect(m_drawWorker, &DrawWorker::imageRendered, this, &MVSCamera::onImageRendered);

    m_drawThread->start();

    // 初始化显示定时器（控制显示刷新率）
    m_displayTimer = new QTimer(this);
    m_displayTimer->setInterval(6); // 约166fps
    connect(m_displayTimer, &QTimer::timeout, this, &MVSCamera::updateDisplay);
    m_displayTimer->start();
}

MVSCamera::~MVSCamera()
{
    // 停止取流
    if (handle) {
        on_Stop_clicked();
    }

    // 停止绘制线程
    if (m_drawThread) {
        m_drawThread->quit();
        m_drawThread->wait();
    }

    delete ui;
}

void MVSCamera::initWindow()
{
    QList<QScreen*>screens =QApplication::screens();
    if(screens.size()>1)
    {
        QRect screenGeometry=screens[0]->geometry();
        move(screenGeometry.topLeft());
//        showMaximized();
        //qDebug()<<size().width()<< size().height();
    }
}

void MVSCamera::showImage(QImage showImage)
{
    // 保存当前视频帧到成员变量
    m_currentVideoFrame = showImage.copy();

    // 将视频帧发送给绘制工作线程
    if (m_drawWorker) {
        m_drawWorker->setVideoFrame(showImage);
        emit requestRender();
    }
}

void __stdcall MVSCamera::ImageCallBack(unsigned char *pData, MV_FRAME_OUT_INFO_EX *pFrameInfo, void *pUser)
{
    MVSCamera* pThis = (MVSCamera*)pUser;
    QImage showImage = QImage(pData, pFrameInfo->nWidth, pFrameInfo->nHeight, QImage::Format_RGB888);
    pThis->showImage(showImage);
}

void MVSCamera::Initialize()
{
    //#1 初始化SDk
    nRet = MV_CC_Initialize();
    if(MV_OK != nRet)
    {
        qDebug() << "SDK Initialize fail!";
        return;
    }

    //#2 枚举设备
    MV_CC_DEVICE_INFO_LIST stDeviceList;
    nRet = MV_CC_EnumDevices(MV_GIGE_DEVICE, &stDeviceList);
    if(MV_OK != nRet)
    {
        qDebug() << "Enum Devices fail!";
        return;
    }

    if(stDeviceList.nDeviceNum > 0)
    {
        for (unsigned int i = 0; i < stDeviceList.nDeviceNum; i++)
        {
            MV_CC_DEVICE_INFO* pDeviceInfo = stDeviceList.pDeviceInfo[i];
            int nIp1 = ((pDeviceInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0xff000000) >> 24);
            int nIp2 = ((pDeviceInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0x00ff0000) >> 16);
            int nIp3 = ((pDeviceInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0x0000ff00) >> 8);
            int nIp4 = (pDeviceInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0x000000ff);
            qDebug("[Device %d]:CurrentIp: %d.%d.%d.%d", i, nIp1, nIp2, nIp3, nIp4);
            if (NULL == pDeviceInfo)
            {
                break;
            }
        }
    }
    else
    {
        qDebug() << "Find No Devices!";
        return;
    }

    //#3 创建句柄
    nRet = MV_CC_CreateHandle(&handle, stDeviceList.pDeviceInfo[0]);
    if (MV_OK != nRet)
    {
        qDebug() << "Create Handle fail!";
        return;
    }

    //#4 打开设备
    nRet = MV_CC_OpenDevice(handle);
    if(MV_OK != nRet)
    {
        qDebug() << "Open Device fail!";
        return;
    }

    //#5 获得网络最佳包大小
    int nPacketSize = MV_CC_GetOptimalPacketSize(handle);
    if (nPacketSize > 0)
    {
        nRet = MV_CC_SetIntValueEx(handle, "GevSCPSPacketSize", nPacketSize);
        if(nRet != MV_OK)
        {
            qDebug() << "Warning: Set Packet Size fail!";
        }
    }
    else
    {
        qDebug() << "Warning: Get Packet Size fail!";
    }

    //#6 关闭触发模式
    nRet = MV_CC_SetEnumValue(handle, "TriggerMode", 0);
    if (MV_OK != nRet)
    {
        qDebug() << "Set Trigger Mode off fail!";
    }

    //#7 注册回调函数
    nRet = MV_CC_RegisterImageCallBackForRGB(handle, ImageCallBack, this);
    if(MV_OK != nRet)
    {
        qDebug() << "RegisterImageCallBackForRGB fail!";
    }
}

void MVSCamera::on_Preview_clicked()
{
    Initialize();

    //#1 开始取流
    nRet = MV_CC_StartGrabbing(handle);
    if (MV_OK != nRet)
    {
        qDebug() << "Start Grabbing fail!";
    }
}

void MVSCamera::on_Stop_clicked()
{
    if (!handle) return;

    //#1 停止取流
    nRet = MV_CC_StopGrabbing(handle);
    if (MV_OK != nRet)
    {
        qDebug() << "Stop Grabbing fail!";
    }

    //#2 关闭设备
    nRet = MV_CC_CloseDevice(handle);
    if (MV_OK != nRet)
    {
        qDebug() << "Close Device fail!";
    }

    //#3 销毁句柄
    nRet = MV_CC_DestroyHandle(handle);
    if (MV_OK != nRet)
    {
        qDebug() << "Destroy Handle fail!";
    }

    //#4 反初始化
    nRet = MV_CC_Finalize();
    if (MV_OK != nRet)
    {
        qDebug() << "Finalize fail!";
    }
    handle = NULL;
}

void MVSCamera::on_Capture_clicked()
{
    if(m_displayImage.isNull())
    {
        QMessageBox::warning(this, "warning", "Cannot save pictures!");
        return;
    }

    QString savePathHead = QDir::rootPath() + "QtMVpicture/";
    QDir dir(savePathHead);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QString curDate = QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss.zzz");
    QString format = "bmp";
    QString savePath = savePathHead + curDate + "." + format;

    // 保存绘制后的图像
    QPixmap picturePixmap = QPixmap::fromImage(m_displayImage);

    // 添加水印
    QPainter painter(&picturePixmap);
    QFont font("Arial", 16, QFont::Bold);
    painter.setFont(font);
    painter.setPen(Qt::white);
    painter.setBrush(QColor(0, 0, 0, 128));
    int padding = 10;
    QFontMetrics fm(font);
    int textWidth = fm.horizontalAdvance(curDate);
    painter.drawText(picturePixmap.width() - textWidth - padding,
                     picturePixmap.height() - padding,
                     curDate);
    painter.end();

    if(picturePixmap.save(savePath))
    {
        QMessageBox::information(this, "Success", "Saved to " + savePath);
    }
    else
    {
        QMessageBox::warning(this, "Error", "Failed to save picture");
    }
}

// 绘制线程返回的渲染结果
void MVSCamera::onImageRendered(const QImage &image)
{
    if (!image.isNull()) {
        m_displayImage = image;
    }
}

// 定时更新显示
void MVSCamera::updateDisplay()
{
    if (!m_displayImage.isNull()) {
        QPixmap showPixmap = QPixmap::fromImage(m_displayImage)
            .scaled(QSize(2560, 1400), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        ui->Camera->setPixmap(showPixmap);
    }
}

// 鼠标事件处理
void MVSCamera::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_isMeasuring)
    {
        m_currentStart = event->pos();
        m_currentEnd = event->pos();
        m_isDrawing = true;

        // 更新绘制线程中的当前线
        if (m_drawWorker) {
            m_drawWorker->updateCurrentLine(m_currentStart, m_currentEnd);
            emit requestRender();
        }
    }

    QMainWindow::mousePressEvent(event);
}

void MVSCamera::mouseMoveEvent(QMouseEvent *event)
{
    if (m_isDrawing)
    {
        m_currentEnd = event->pos();

        // 更新绘制线程中的当前线
        if (m_drawWorker) {
            m_drawWorker->updateCurrentLine(m_currentStart, m_currentEnd);
            emit requestRender();
        }
    }

    QMainWindow::mouseMoveEvent(event);
}

void MVSCamera::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_isDrawing)
    {
        m_currentEnd = event->pos();
        m_isDrawing = false;

        // 只有当起点和终点不同时才保存
        if (m_currentStart != m_currentEnd)
        {
            double dist = calculateDistance(m_currentStart, m_currentEnd);

            // 将完成的线添加到绘制线程
            if (m_drawWorker) {
                m_drawWorker->addLine(m_currentStart, m_currentEnd, dist);
                emit requestRender();
            }
        }
    }

    QMainWindow::mouseReleaseEvent(event);
}

double MVSCamera::calculateDistance(const QPoint &p1, const QPoint &p2)
{
    double dx = p2.x() - p1.x();
    double dy = p2.y() - p1.y();
    return std::sqrt(dx * dx + dy * dy);
}

void MVSCamera::on_measure_clicked()
{
    m_isMeasuring = !m_isMeasuring;
    if (m_isMeasuring) {
        ui->measure->setText("Stop Measure");
    } else {
        ui->measure->setText("Start Measure");
    }
}

void MVSCamera::on_clear_clicked()
{
    if (m_drawWorker) {
        m_drawWorker->clearAllLines();
        emit requestRender();
    }
}
