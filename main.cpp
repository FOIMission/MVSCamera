#include "mvscamera.h"
#include <QApplication>

void loadQss(QWidget* widget, const QString& path) {
    QFile file(path);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        widget->setStyleSheet(file.readAll());
        file.close();
    } else {
        qDebug() << "Failed to load style sheet:" << path;
    }
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MVSCamera MVSCamera_w;
    loadQss(&MVSCamera_w,":/qss/buttons.qss");
    MVSCamera_w.show();
    return a.exec();
}
