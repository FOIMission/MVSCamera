#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <mycamera.h>
#include <QThread>
QT_BEGIN_NAMESPACE
namespace Ui { class Mainwindow; }
QT_END_NAMESPACE

class Mainwindow : public QMainWindow
{
    Q_OBJECT

public:
    Mainwindow(QWidget *parent = nullptr);
    ~Mainwindow();


    QWidget* drawingWidget;
    QWidget* containerWidget;
protected:
        Ui::Mainwindow *ui;

private slots:
    void on_Preview_clicked();
    void on_Stop_clicked();
    void on_Capture_clicked();


private:
    myCamera* myCamera1;
    QThread* CameraThread;
    void initWindow();
    void setupCameraThread();
    void showImage(const QImage &showImage);
};
#endif // MAINWINDOW_H
