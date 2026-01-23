#ifndef MVSCAMERA_H
#define MVSCAMERA_H

#include <QMainWindow>
#include "MvCameraControl.h"
#include <QImage>
#include <QPixmap>
#include <QDateTime>
#include <QMessageBox>
#include <QDir>
#include <QDialog>
#include <QDebug>
#include <QMessageBox>
#include <QErrorMessage>
#include <QApplication>
#include <QPainter>
#include <QScreen>
#include <QFileDialog>
#include <QFileInfo>
#include <QTreeWidget>
#include <QMap>
QT_BEGIN_NAMESPACE
namespace Ui { class MVSCamera; }
QT_END_NAMESPACE

class MVSCamera : public QMainWindow
{
    Q_OBJECT

public:
    MVSCamera(QWidget *parent = nullptr);
    ~MVSCamera();

    int nRet = MV_OK;
    void * handle=NULL;

protected:
        Ui::MVSCamera *ui;

private slots:
    void on_Preview_clicked();
    void on_Stop_clicked();
    void on_Capture_clicked();
    void on_selectFilePath_clicked();

private:
    QImage myImage;
    QTimer *DeviceMonitorTimer;
    QTreeWidget *deviceTreeWidget;
    QMap<QString, MV_CC_DEVICE_INFO*> deviceInfoMap;
    MV_CC_DEVICE_INFO* deviceInfotmp=nullptr;
    bool isInitial=false;
    bool isPreviewing=false;
    bool isPausing=false;
    QString strFilePath;
    void showImage(QImage Image);
    static void __stdcall ImageCallBack (unsigned char *pData, MV_FRAME_OUT_INFO_EX *pFrameInfo, void *pUser);
    bool Initialize();
    void InitWindow();
    void InitSignalsConnect();
    void checkDevices();
};
#endif // MVSCAMERA_H
