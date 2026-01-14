#include "mvscamera.h"
#include "ui_mvscamera.h"

MVSCamera::MVSCamera(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MVSCamera)
{
    ui->setupUi(this);
    this->setWindowTitle("Camera");
    initWindow();
}

MVSCamera::~MVSCamera()
{
    delete ui;
}

void MVSCamera::initWindow()
{
    // 获取所有屏幕
    QList<QScreen*> screens = QApplication::screens();

    if (screens.size() > 1) {
        QRect screenGeometry = screens[0]->geometry();  // 第一个屏幕
        // 移动到目标屏幕
        move(screenGeometry.topLeft());
        // 最大化窗口（保留标题栏和按钮）
        showMaximized();
        //qDebug()<<size().width()<< size().height();
    }
}

void MVSCamera::showImage(QImage showImage)
{
    QPixmap showPixmap = QPixmap::fromImage(showImage).scaled(QSize(2560,1400),Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
    ui->Camera->setPixmap(showPixmap);
}

void __stdcall MVSCamera:: ImageCallBack (unsigned char *pData, MV_FRAME_OUT_INFO_EX *pFrameInfo, void *pUser)
{
//    QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss:zzz");可以放到label
    MVSCamera* pThis = (MVSCamera*)pUser;
    QImage showImage = QImage(pData, pFrameInfo->nWidth,pFrameInfo->nHeight,QImage::Format_RGB888);
    pThis->showImage(showImage);
}

void MVSCamera::Initialize()
{
    //#1 初始化SDk
    nRet=MV_CC_Initialize();
    if(MV_OK!=nRet)
    {
        qDebug()<<"SDK Initialize fail!";
    }

    //#2 枚举设备
    MV_CC_DEVICE_INFO_LIST stDeviceList;
    nRet=MV_CC_EnumDevices(MV_GIGE_DEVICE,&stDeviceList);
    if(MV_OK!=nRet)
    {
        qDebug()<<"Enum Devices fail!";
    }
    if(stDeviceList.nDeviceNum>0)
    {
        for (unsigned int i=0;i<stDeviceList.nDeviceNum;i++)
        {
            MV_CC_DEVICE_INFO* pDeviceInfo = stDeviceList.pDeviceInfo[i];
            int nIp1 = ((pDeviceInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0xff000000) >> 24);
            int nIp2 = ((pDeviceInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0x00ff0000) >> 16);
            int nIp3 = ((pDeviceInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0x0000ff00) >> 8);
            int nIp4 = (pDeviceInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0x000000ff);
            qDebug("[Device %d]:CurrentIp: %d.%d.%d.%d",i,nIp1,nIp2,nIp3,nIp4);
            if (NULL == pDeviceInfo)
            {
                break;
            }
        }
    }
    else
    {
        qDebug()<<"Find No Devices!";
    }

    //#3 创建句柄
    nRet=MV_CC_CreateHandle(&handle,stDeviceList.pDeviceInfo[0]);
    if (MV_OK != nRet)
    {
        qDebug()<<"Create Handle fail!";
    }

    //#4 打开设备
    nRet = MV_CC_OpenDevice(handle);
    if(MV_OK!=nRet)
    {
        qDebug()<<"Open Device fail!";
    }

    //#5 获得网络最佳包大小
    int nPacketSize = MV_CC_GetOptimalPacketSize(handle);
    if (nPacketSize > 0)
    {
        nRet = MV_CC_SetIntValueEx(handle,"GevSCPSPacketSize",nPacketSize);
        if(nRet != MV_OK)
        {
            qDebug()<<"Warning: Set Packet Size fail!";
        }
    }
    else
    {
        qDebug()<<"Warning: Get Packet Size fail!";
    }

    //#6 关闭触发模式
    nRet = MV_CC_SetEnumValue(handle, "TriggerMode", 0);
    if (MV_OK != nRet)
    {
        qDebug()<<"Set Trigger Mode off fail!";
    }

    //#7 注册回调函数
    nRet =MV_CC_RegisterImageCallBackForRGB(handle,ImageCallBack,this);
   if(MV_OK != nRet)
   {
       qDebug()<<"RegisterImageCallBackForRGB fail!";
   }
}

void MVSCamera::on_Preview_clicked()
{
    Initialize();
    //#1 开始取流
    nRet = MV_CC_StartGrabbing(handle);
    if (MV_OK != nRet)
    {
        qDebug()<<"Start Grabbing fail!";
    }
}

void MVSCamera::on_Stop_clicked()
{
    //#1 停止取流
    nRet = MV_CC_StopGrabbing(handle);
    if (MV_OK != nRet)
    {
        qDebug()<<"Stop Grabbing fail!";
    }
    //#2 关闭设备
    nRet = MV_CC_CloseDevice(handle);
    if (MV_OK != nRet)
    {
        qDebug()<<"Close Device fail!";
    }
    //#3 销毁句柄
    nRet = MV_CC_DestroyHandle(handle);
    if (MV_OK != nRet)
    {
        qDebug()<<"Destroy Handle fail!";
    }
    //#4 反初始化
    nRet = MV_CC_Finalize();
    if (MV_OK != nRet)
    {
        qDebug()<<"Finalize fail!";
    }
    handle = NULL;
}

void MVSCamera::on_Capture_clicked()
{
    //#1 捕获
    if(ui->Camera->pixmap()==NULL)
    {
        QMessageBox::warning(this,"warning","Cannot save pictures!");
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

