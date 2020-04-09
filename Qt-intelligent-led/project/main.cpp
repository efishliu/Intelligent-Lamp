#include "widget.h"
#include <QApplication>
#include <QFile>
#include <QtGui>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
//------------加载qss-------------
  QFile qss(":/led/src/led2.qss");
    qss.open(QFile::ReadOnly);
    qApp->setStyleSheet(qss.readAll());
    qss.close();

    Widget w;
    w.show();


    return a.exec();
}
