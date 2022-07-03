#include "mainwindow.h"

#include <QApplication>
#include <QFile>
#include <qnamespace.h>
int main(int argc, char *argv[])
{
    setvbuf(stdout, NULL, _IONBF, 0);
    QApplication a(argc, argv);
    MainWindow w;

    QFile file(":/css/default.css");
    file.open(QFile::ReadOnly);
    QString stylesheet = file.readAll();
    file.close();
    a.setStyleSheet(stylesheet);

    w.show();
    return a.exec();
}
