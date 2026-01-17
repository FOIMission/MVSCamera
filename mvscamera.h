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
#include <QErrorMessage>
#include <QApplication>
#include <QPainter>
#include <QScreen>
#include <QThread>
#include <QTimer>
#include <QMouseEvent>
#include <cmath>

QT_BEGIN_NAMESPACE
namespace Ui { class MVSCamera; }
QT_END_NAMESPACE

// 前向声明
class DrawWorker;

class MVSCamera : public QMainWindow
{
    Q_OBJECT

public:
    MVSCamera(QWidget *parent = nullptr);
    ~MVSCamera();

    int nRet = MV_OK;
    void * handle = NULL;

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private slots:
    void on_Preview_clicked();
    void on_Stop_clicked();
    void on_Capture_clicked();
    void onImageRendered(const QImage &image);
    void updateDisplay();
    void on_measure_clicked();
    void on_clear_clicked();

signals:
    void requestRender();

private:
    Ui::MVSCamera *ui;

    void showImage(QImage showImage);
    static void __stdcall ImageCallBack(unsigned char *pData, MV_FRAME_OUT_INFO_EX *pFrameInfo, void *pUser);
    void Initialize();
    void initWindow();

    // 绘制相关成员
    DrawWorker *m_drawWorker;
    QThread *m_drawThread;
    QImage m_currentVideoFrame;
    QImage m_displayImage;
    QTimer *m_displayTimer;

    // 测量相关成员
    bool m_isMeasuring;
    bool m_isDrawing;
    QPoint m_currentStart;
    QPoint m_currentEnd;

    // 辅助函数
    double calculateDistance(const QPoint &p1, const QPoint &p2);
};

#endif // MVSCAMERA_H
