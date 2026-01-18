#include "mvscamera.h"
#include "ui_mvscamera.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
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
    if(handle)
    {
        on_Stop_clicked();
    }
    delete ui;
}

void MVSCamera::initWindow()
{
    // 获取所有屏幕(适用于多个屏幕)
    QList<QScreen*> screens = QApplication::screens();
    if (screens.size() > 1) {
        QRect screenGeometry = screens[0]->geometry();
        move(screenGeometry.topLeft());
        showMaximized();
        //qDebug()<<size().width()<< size().height();
    }
    ui->Camera->setStyleSheet("background-color: #1e1e1e;");
    ui->Camera->setAlignment(Qt::AlignCenter);
    ui->Camera->setGeometry(0, 0, 1920, 1080);
    ui->Camera->setMinimumSize(1920, 1080);
    ui->Camera->setMaximumSize(1920, 1080);
    ui->Preview->setCheckable(true);

    QVBoxLayout *verticalLayout = new QVBoxLayout;
    verticalLayout->addWidget(ui->Camera);
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->setSpacing(0);
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->addWidget(ui->Preview);
    buttonLayout->addWidget(ui->Stop);
    buttonLayout->addWidget(ui->Capture);
    buttonLayout->addWidget(ui->MarkComboBox);
    buttonLayout->addWidget(ui->selectFilePath);
    buttonLayout->addStretch();
    verticalLayout->addLayout(buttonLayout);
    verticalLayout->addStretch();
    centralWidget()->setLayout(verticalLayout);
}

void MVSCamera::showImage(QImage Image)
{
    myImage=Image;
    QPixmap showPixmap = QPixmap::fromImage(myImage).scaled(QSize(ui->Camera->width(),ui->Camera->height()),Qt::KeepAspectRatio,Qt::SmoothTransformation);
    ui->Camera->setPixmap(showPixmap);
}

void __stdcall MVSCamera:: ImageCallBack (unsigned char *pData, MV_FRAME_OUT_INFO_EX *pFrameInfo, void *pUser)
{
    MVSCamera* pThis = (MVSCamera*)pUser;
    QImage myImageTmp = QImage(pData, pFrameInfo->nWidth,pFrameInfo->nHeight,QImage::Format_RGB888);
    pThis->showImage(myImageTmp);
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

    //#2 枚举设备
    MV_CC_DEVICE_INFO_LIST stDeviceList;
    nRet=MV_CC_EnumDevices(MV_GIGE_DEVICE,&stDeviceList);
    if(MV_OK!=nRet)
    {
        qDebug()<<"Enum Devices fail!";
        return false;
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
        return false;
    }

    //#3 创建句柄
    nRet=MV_CC_CreateHandle(&handle,stDeviceList.pDeviceInfo[0]);
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

void MVSCamera::on_Preview_clicked()
{
    if(!isInitial)
    {
        if(!Initialize())
        {
            qDebug()<<"Init failed!";
            return;
        }
        isInitial = true;
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

void MVSCamera::on_Stop_clicked()
{
    //#1 停止取流
    if(isInitial&&isPreviewing)
    {
        nRet = MV_CC_StopGrabbing(handle);
        if (MV_OK != nRet)
        {
            qDebug()<<"Stop Grabbing fail!";
            return;
        }
    }
    //#2 关闭设备
    nRet = MV_CC_CloseDevice(handle);
    if (MV_OK != nRet)
    {
        qDebug()<<"Close Device fail!";
        return;
    }
    //#3 销毁句柄
    nRet = MV_CC_DestroyHandle(handle);
    if (MV_OK != nRet)
    {
        qDebug()<<"Destroy Handle fail!";
        return;
    }
    //#4 反初始化
    nRet = MV_CC_Finalize();
    if (MV_OK != nRet)
    {
        qDebug()<<"Finalize fail!";
        return;
    }
    handle = NULL;
    isInitial=false;
    isPreviewing=false;
    isPausing=false;
    ui->Preview->setChecked(false);
    //ui->Preview->setText(u8"预览");
    ui->Camera->clear();
}

void MVSCamera::on_Capture_clicked()
{
    //#1 捕获
    if(!handle)
    {
        QMessageBox::warning(this,"warning","Cannot save pictures!");
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
    QPixmap mypixmap = QPixmap::fromImage(myImage);

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
        u8"选择要保存的文件夹",            // 对话框标题
        QDir::currentPath(), // 默认路径为当前工作目录
        QFileDialog::ShowDirsOnly // 只显示目录
    );

}
