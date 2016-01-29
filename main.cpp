#include "mainwindow.h"
#include <QApplication>

#ifdef Q_OS_WIN32
#include "vld/vld.h"
#endif

int main(int argc, char *argv[])
{
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}
