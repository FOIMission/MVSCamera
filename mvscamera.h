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
#include <QThread>
#include "graphicsdrawview.h"
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
    void * handle=nullptr;
protected:
        Ui::MVSCamera *ui;

private slots:
    void on_Preview_clicked();
    void on_Capture_clicked();
    void on_selectFilePath_clicked();

    void on_measure_clicked();

    void on_clearLines_clicked();
    void updateImage(const QImage &image);

    void onInitializeFinished(bool success, QTreeWidgetItem* item);
    void onFinalizeFinished(bool success, QTreeWidgetItem* item);
signals:
    void newImageReady(const QImage &image);  // 相机线程图片信号
    void initializeFinished(bool success, QTreeWidgetItem* item);//异步初始化线程结束信号
    void finalizeFinished(bool success, QTreeWidgetItem* item);//异步反初始化线程结束信号

private:
    QImage myImage;
    QTimer *DeviceMonitorTimer;
    QTreeWidget *deviceTreeWidget;
    QMap<QString, MV_CC_DEVICE_INFO> deviceInfoMaptmp;
    QThread *workerThread;

    MV_CC_DEVICE_INFO* deviceInfo=nullptr;
    bool isInitial=false;
    bool isPreviewing=false;
    bool isPausing=false;
    QString strFilePath;

    void showImage(QImage Image);
    static void __stdcall ImageCallBack (unsigned char *pData, MV_FRAME_OUT_INFO_EX *pFrameInfo, void *pUser);
    bool Initialize();
    bool Finalize();
    void InitWindow();
    void InitSignalsConnect();
    void updateDeviceList(const QMap<QString, QString>& scannedDevices, const QSet<QString>& scannedIPs, QMap<QString, MV_CC_DEVICE_INFO> deviceInfoMap);

    GraphicsDrawView *m_drawView;
};
#endif // MVSCAMERA_H
