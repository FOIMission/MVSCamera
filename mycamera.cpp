#include "mycamera.h"
#include <QMutexLocker>
myCamera::myCamera(QObject *parent)
    :QObject(parent)
{

}

myCamera::~myCamera()
{

}

void __stdcall myCamera:: ImageCallBack (unsigned char *pData, MV_FRAME_OUT_INFO_EX *pFrameInfo, void *pUser)
{
    myCamera* pThis = (myCamera*)pUser;
    pThis->ProcessImage(pData, pFrameInfo);
}

void myCamera::ProcessImage(unsigned char *pData, MV_FRAME_OUT_INFO_EX *pFrameInfo)
{
    QImage showImage = QImage(pData, pFrameInfo->nWidth,pFrameInfo->nHeight,QImage::Format_RGB888);
    emit ImageReady(showImage.copy());
}

void myCamera::Initialize()
{
    QMutexLocker locker(&m_mutex);
    //#1 初始化SDk
    nRet=MV_CC_Initialize();
    if(MV_OK!=nRet)
    {
        emit errorSignal("SDK Initialize fail!");
    }

    //#2 枚举设备
    MV_CC_DEVICE_INFO_LIST stDeviceList;
    nRet=MV_CC_EnumDevices(MV_GIGE_DEVICE,&stDeviceList);
    if(MV_OK!=nRet)
    {
        emit errorSignal("Enum Devices fail!");
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
        emit errorSignal("Find No Devices!");
    }

    //#3 创建句柄
    nRet=MV_CC_CreateHandle(&handle,stDeviceList.pDeviceInfo[0]);
    if (MV_OK != nRet)
    {
        emit errorSignal("Create Handle fail!");
    }

    //#4 打开设备
    nRet = MV_CC_OpenDevice(handle);
    if(MV_OK!=nRet)
    {
        emit errorSignal("Open Device fail!");
    }

    //#5 获得网络最佳包大小
    int nPacketSize = MV_CC_GetOptimalPacketSize(handle);
    if (nPacketSize > 0)
    {
        nRet = MV_CC_SetIntValueEx(handle,"GevSCPSPacketSize",nPacketSize);
        if(nRet != MV_OK)
        {
            emit errorSignal("Warning: Set Packet Size fail!");
        }
    }
    else
    {
        emit errorSignal("Warning: Get Packet Size fail!");
    }

    //#6 关闭触发模式
    nRet = MV_CC_SetEnumValue(handle, "TriggerMode", 0);
    if (MV_OK != nRet)
    {
        emit errorSignal("Set Trigger Mode off fail!");
    }

    //#7 注册回调函数
    nRet =MV_CC_RegisterImageCallBackForRGB(handle,ImageCallBack,this);
   if(MV_OK != nRet)
   {
       emit errorSignal("RegisterImageCallBackForRGB fail!");
   }
}

void myCamera::Preview()
{
    QMutexLocker locker(&m_mutex);
    Initialize();
    //#1 开始取流
    nRet = MV_CC_StartGrabbing(handle);
    if (MV_OK != nRet)
    {
        emit errorSignal("Start Grabbing fail!");
    }
    emit Previewing();
}

void myCamera::Stop()
{
    //#1 停止取流
    nRet = MV_CC_StopGrabbing(handle);
    if (MV_OK != nRet)
    {
        emit errorSignal("Stop Grabbing fail!");
    }
    //#2 关闭设备
    nRet = MV_CC_CloseDevice(handle);
    if (MV_OK != nRet)
    {
        emit errorSignal("Close Device fail!");
    }
    //#3 销毁句柄
    nRet = MV_CC_DestroyHandle(handle);
    if (MV_OK != nRet)
    {
        emit errorSignal("Destroy Handle fail!");
    }
    //#4 反初始化
    nRet = MV_CC_Finalize();
    if (MV_OK != nRet)
    {
        emit errorSignal("Finalize fail!");
    }
    handle = NULL;
}


