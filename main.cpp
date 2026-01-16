#include "Mainwindow.h"
#include "mydismeasuring_pen.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Mainwindow Mainwindow_w;
    Mainwindow_w.show();
    Mydismeasuring_pen Mydismeasuring_pen_w;
    Mydismeasuring_pen_w.show();
    return a.exec();
}
