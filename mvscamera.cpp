#include "mvscamera.h"
#include "ui_mvscamera.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QListWidget>
#include <QTreeWidget>
#include <QHeaderView>
#include <QTimer>
#include <QtConcurrent/QtConcurrentRun>
#include "DeviceMonitorWorker.h"
MVSCamera::MVSCamera(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MVSCamera)
{
    ui->setupUi(this);
    this->setWindowTitle("Camera");
    InitWindow();
    InitSignalsConnect();
}

MVSCamera::~MVSCamera()
{
    if(handle)
    {
        Finalize();
    }
    if (workerThread && workerThread->isRunning()) {
           workerThread->quit();
           workerThread->wait();
       }
    delete ui;
}

void MVSCamera::InitWindow()
{
    // 获取所有屏幕(适用于多个屏幕)
    QList<QScreen*> screens = QApplication::screens();
    if (screens.size() >= 1) {
        QRect screenGeometry = screens[0]->geometry();
        move(screenGeometry.topLeft());
        showMaximized();
        //qDebug()<<size().width()<< size().height();
    }

    m_drawView = new GraphicsDrawView(this);
    m_drawView->setObjectName("drawView");  // 可选
    // 设置视图背景与图像适应等
    m_drawView->setBackgroundBrush(Qt::black);
    m_drawView->setAlignment(Qt::AlignCenter);
    m_drawView->setGeometry(0, 0, 1920, 1080);
    m_drawView->setMinimumSize(1920, 1080);
    m_drawView->setMaximumSize(1920, 1080);
    ui->Preview->setCheckable(true);

    QVBoxLayout *verticalLayout = new QVBoxLayout;
    verticalLayout->addWidget(m_drawView);
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->setSpacing(0);
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->addWidget(ui->Preview);
    buttonLayout->addWidget(ui->Capture);
    buttonLayout->addWidget(ui->MarkComboBox);
    buttonLayout->addWidget(ui->selectFilePath);
    buttonLayout->addWidget(ui->measure);
    buttonLayout->addWidget(ui->clearLines);
    buttonLayout->addStretch();
    verticalLayout->addLayout(buttonLayout);
    verticalLayout->addStretch();

    QHBoxLayout *centralHboxwidget=new QHBoxLayout;//我把整个ui以横着的方式排列，目前分为两部分
    centralHboxwidget->addLayout(verticalLayout);//这是这个横排列的第一个layout，是一个占满左半部分的竖排列

    QGroupBox *deviceGroupBox = new QGroupBox(u8"设备列表");
    QVBoxLayout *deviceLayout = new QVBoxLayout;
    // 创建设备列表
    deviceTreeWidget = new QTreeWidget;
    deviceTreeWidget->setColumnCount(3);
    deviceTreeWidget->setHeaderLabels({u8"设备名称", u8"IP", u8"选中状态"});
    deviceTreeWidget->setColumnWidth(0, 150);
    deviceTreeWidget->setColumnWidth(1, 180);
    deviceTreeWidget->setColumnWidth(2, 100);
    deviceTreeWidget->setFixedWidth(430);//waring:存在无法自适应的问题
    deviceTreeWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    deviceLayout->addWidget(deviceTreeWidget);
    deviceGroupBox->setLayout(deviceLayout);
    QVBoxLayout *deviceGroupLayout = new QVBoxLayout;//这是这个横排列的第二个layout，是一个占满右半部分的竖排列
    deviceGroupLayout->addWidget(deviceGroupBox);
    deviceGroupLayout->addStretch();
    centralHboxwidget->addLayout(deviceGroupLayout);
    centralHboxwidget->addStretch();
    centralWidget()->setLayout(centralHboxwidget);
    ui->Camera->hide();

    connect(this, &MVSCamera::initializeFinished, this, &MVSCamera::onInitializeFinished, Qt::QueuedConnection);
    connect(this, &MVSCamera::finalizeFinished, this, &MVSCamera::onFinalizeFinished, Qt::QueuedConnection);

    QObject::connect(deviceTreeWidget, &QTreeWidget::itemChanged, this, [this](QTreeWidgetItem *item, int column) {
            if (column != 2) return;
            if (item->checkState(2) == Qt::Checked) {
                deviceInfo = &deviceInfoMaptmp[item->text(1)];
                if (!isInitial) {
                    // 临时禁用该复选框，防止在异步操作期间被再次点击
                    item->setFlags(item->flags() & ~Qt::ItemIsUserCheckable);
                    isInitial = true;
                    // 异步执行 Initialize
                    QtConcurrent::run([this, item]() {
                        bool ok = Initialize();
                        emit initializeFinished(ok, item);
                    });
                }
            } else {
                if (isInitial) {
                    // 临时禁用该复选框
                    item->setFlags(item->flags() & ~Qt::ItemIsUserCheckable);
                    isInitial = false;
                    // 异步执行 Finalize
                    QtConcurrent::run([this, item]() {
                        bool ok = Finalize();
                        emit finalizeFinished(ok, item);
                    });
                }
            }
        });
}

void MVSCamera::InitSignalsConnect()
{
    workerThread = new QThread(this);
    DeviceMonitorWorker *worker = new DeviceMonitorWorker();

    worker->moveToThread(workerThread);

    // 连接工作对象的信号到主线程的UI更新槽
    connect(worker, &DeviceMonitorWorker::deviceMessage, this, &MVSCamera::updateDeviceList, Qt::QueuedConnection);
    // 控制线程启动/停止
    connect(workerThread, &QThread::started, worker, &DeviceMonitorWorker::startMonitoring);
    connect(workerThread, &QThread::finished, worker, &DeviceMonitorWorker::deleteLater);
    connect(workerThread, &QThread::finished, workerThread, &QThread::deleteLater);

    // 将相机线程的图像信号连接到主线程的更新槽
    connect(this, &MVSCamera::newImageReady, this, &MVSCamera::updateImage, Qt::QueuedConnection);
    // 启动线程
    workerThread->start();
}

void MVSCamera::updateDeviceList(const QMap<QString, QString>& scannedDevices, const QSet<QString>& scannedIPs, QMap<QString, MV_CC_DEVICE_INFO> deviceInfoMap)
{
    deviceTreeWidget->clear();
    deviceInfoMaptmp=deviceInfoMap;//info中介
    // 添加
    for (const QString &ip : scannedIPs) {
        QString deviceName = scannedDevices[ip];
        QTreeWidgetItem *deviceItem = new QTreeWidgetItem(deviceTreeWidget);
        deviceItem->setText(0, deviceName);
        deviceItem->setText(1, ip);
        deviceItem->setCheckState(2, Qt::Unchecked);
    }

    // 如果没有设备则清空
    if (scannedIPs.isEmpty() && deviceTreeWidget->topLevelItemCount() > 0) {
        deviceTreeWidget->clear();
    }
}

void MVSCamera::showImage(QImage Image)
{
    myImage = Image;
    QPixmap showPixmap = QPixmap::fromImage(myImage).scaled(QSize(ui->Camera->width(), ui->Camera->height()), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    ui->Camera->setPixmap(showPixmap);
}

void __stdcall MVSCamera::ImageCallBack(unsigned char *pData, MV_FRAME_OUT_INFO_EX *pFrameInfo, void *pUser)
{
    MVSCamera* pThis = (MVSCamera*)pUser;
    QImage myImageTmp = QImage(pData, pFrameInfo->nWidth, pFrameInfo->nHeight, QImage::Format_RGB888);
    emit pThis->newImageReady(myImageTmp.copy());
}

void MVSCamera::updateImage(const QImage &image)
{
    if (m_drawView) {
        m_drawView->setImage(image);
    }
}

bool MVSCamera::Initialize()
{
    //#1 初始化SDk
    nRet=MV_CC_Initialize();
    if(MV_OK!=nRet)
    {
        qDebug()<<"SDK Initialize fail!";
        return false;
    }

    if(deviceInfo==nullptr)
    {
        qDebug()<<u8"未选中设备";
        return false;
    }

    //#3 创建句柄
    nRet=MV_CC_CreateHandle(&handle,deviceInfo);
    if (MV_OK != nRet)
    {
        qDebug()<<"Create Handle fail!";
        return false;
    }

    //#4 打开设备
    nRet = MV_CC_OpenDevice(handle);
    if(MV_OK!=nRet)
    {
        qDebug()<<"Open Device fail!";
        return false;
    }

    //#5 获得网络最佳包大小
    int nPacketSize = MV_CC_GetOptimalPacketSize(handle);
    if (nPacketSize > 0)
    {
        nRet = MV_CC_SetIntValueEx(handle,"GevSCPSPacketSize",nPacketSize);
        if(nRet != MV_OK)
        {
            qDebug()<<"Warning: Set Packet Size fail!";
            return false;
        }
    }
    else
    {
        qDebug()<<"Warning: Get Packet Size fail!";
        return false;
    }

    //#6 关闭触发模式
    nRet = MV_CC_SetEnumValue(handle, "TriggerMode", 0);
    if (MV_OK != nRet)
    {
        qDebug()<<"Set Trigger Mode off fail!";
        return false;
    }

    //#7 注册回调函数
    nRet =MV_CC_RegisterImageCallBackForRGB(handle,ImageCallBack,this);
   if(MV_OK != nRet)
   {
       qDebug()<<"RegisterImageCallBackForRGB fail!";
       return false;
   }
   return true;
}

bool MVSCamera::Finalize()
{
    //#1 停止取流
    if(isInitial&&isPreviewing)
    {
        nRet = MV_CC_StopGrabbing(handle);
        if (MV_OK != nRet)
        {
            qDebug()<<"Stop Grabbing fail!";
            return false;
        }
    }
    //#2 关闭设备
    nRet = MV_CC_CloseDevice(handle);
    if (MV_OK != nRet)
    {
        qDebug()<<"Close Device fail!";
        return false;
    }
    //#3 销毁句柄
    nRet = MV_CC_DestroyHandle(handle);
    if (MV_OK != nRet)
    {
        qDebug()<<"Destroy Handle fail!";
        return false;
    }
    //#4 反初始化
    nRet = MV_CC_Finalize();
    if (MV_OK != nRet)
    {
        qDebug()<<"Finalize fail!";
        return false;
    }
    handle = NULL;
    isPreviewing=false;
    isPausing=false;
    ui->Preview->setChecked(false);
    return true;
}

void MVSCamera::onInitializeFinished(bool success, QTreeWidgetItem* item)
{
    if (!success) {
            QMessageBox::warning(this, "warning", "Device initialization failed");
        }
    if (item) {
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    }
}

void MVSCamera::onFinalizeFinished(bool success, QTreeWidgetItem* item)
{
    if (!success) {
            QMessageBox::warning(this, "warning", "Device Finalization failed");
        }
    if (item) {
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    }
}

void MVSCamera::on_Preview_clicked()
{
    if(!handle){
        ui->Preview->setChecked(false);
        QMessageBox::warning(this, "warning", "No device");
        return;
    }
    if(!isPreviewing)
    {
        //#1 开始取流
        nRet = MV_CC_StartGrabbing(handle);
        if (MV_OK != nRet)
        {
            qDebug()<<"Start Grabbing fail!";
            return;
        }
        //ui->Preview->setText(u8"暂停");
        isPreviewing=true;
        isPausing=false;
    }
    else
    {
        //#1 停止取流
        nRet = MV_CC_StopGrabbing(handle);
        if (MV_OK != nRet)
        {
            qDebug()<<"Stop Grabbing fail!";
            return;
        }
        //ui->Preview->setText(u8"预览");
        isPreviewing=false;
        isPausing=true;
    }
}

void MVSCamera::on_Capture_clicked()
{
    //#1 捕获
    if (handle==nullptr) {
        QMessageBox::warning(this, "warning", "No device");
        return;
    }

    // 获取图像
    QImage Image = m_drawView->renderToImage();
    if (Image.isNull()) {
        QMessageBox::warning(this, "warning", "cannot get Image");
        return;
    }

    QString PathHead;
    if(strFilePath.isEmpty())
    {
        PathHead= QDir::rootPath()+"QtMVpicture/";
        if (!QDir().mkpath(PathHead))
        {
            QMessageBox::warning(this,"warning","Cannot create "+PathHead);
            return;
        }
    }
    else
    {
        PathHead=strFilePath+"/";
    }
    QString curDate = QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss.zzz");
    QString format="bmp";//文件较大，较小用png
    QString savePath = PathHead + curDate + "." + format;
    QPixmap mypixmap = QPixmap::fromImage(Image);

    //#2 贴水印
    QPainter painter(&mypixmap);
    QFont font("Arial", 16, QFont::Bold);
    painter.setFont(font);
    painter.setPen(Qt::white);
    painter.setBrush(QColor(0, 0, 0, 128));
    int padding = 10;
    QFontMetrics fm(font);
    int textWidth = fm.horizontalAdvance(curDate);
    int textHeight = fm.height();

    int xPos = 0, yPos = 0;

    // 根据下拉框选择的位置绘制
    switch(ui->MarkComboBox->currentIndex()) {
        case 0:
            xPos = padding;
            yPos = padding + textHeight;
            break;
        case 1:
            xPos = mypixmap.width() - textWidth - padding;
            yPos = padding + textHeight;
            break;
        case 2:
            xPos = padding;
            yPos = mypixmap.height() - padding;
            break;
        case 3:
            xPos = mypixmap.width() - textWidth - padding;
            yPos = mypixmap.height() - padding;
            break;
        default:
            xPos = mypixmap.width() - textWidth - padding;
            yPos = mypixmap.height() - padding;
            break;
    }

    // 绘制水印文字
    painter.drawText(xPos, yPos, curDate);

//    painter.drawText(mypixmap.width() - textWidth - padding,
//                     mypixmap.height() - padding,
//                     curDate);

    painter.end();
    if(mypixmap.save(savePath))
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

void MVSCamera::on_selectFilePath_clicked()
{
    // 获取单个文件路径
    strFilePath = QFileDialog::getExistingDirectory(
        this,                  // 父窗口
        u8"保存路径",            // 对话框标题
        QDir::currentPath(), // 默认路径为当前工作目录
        QFileDialog::ShowDirsOnly // 只显示目录
    );
}

void MVSCamera::on_measure_clicked()
{
    static bool measuring = false;  // 可以用成员变量代替
    measuring = !measuring;
    if (m_drawView)
    {
        m_drawView->setMeasuring(measuring);
    }
    ui->measure->setText(measuring ? u8"停止测量" : u8"开始测量");
}

void MVSCamera::on_clearLines_clicked()
{
    if (m_drawView)
    {
            m_drawView->clearLines();
    }
}
