#include "mvscamera.h"
#include "mydismeasuring_pen.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MVSCamera MVSCamera_w;
    MVSCamera_w.show();
    return a.exec();
}
