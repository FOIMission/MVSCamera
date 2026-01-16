#ifndef MYCAMERA_H
#define MYCAMERA_H

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
#include <QMutex>
class myCamera :public QObject
{
    Q_OBJECT
public:
    explicit myCamera(QObject *parent = nullptr);
    ~myCamera();

    int nRet = MV_OK;
    void * handle=NULL;
protected:

signals:
    void errorSignal(const QString &errorMessage);
    void ImageReady(const QImage &Image);
    void Previewing();

private slots:
    void Preview();
    void Stop();

private:
    QMutex m_mutex;

    static void __stdcall ImageCallBack (unsigned char *pData, MV_FRAME_OUT_INFO_EX *pFrameInfo, void *pUser);
    void ProcessImage(unsigned char *pData, MV_FRAME_OUT_INFO_EX *pFrameInfo);
    void Initialize();
};

#endif // MYCAMERA_H
