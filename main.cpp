#include "mainwindow.h"
#include <QApplication>

#ifdef Q_OS_WIN32
#include "vld/vld.h"
#endif

#ifdef Q_OS_LINUX
extern QString device;
#endif

int main(int argc, char *argv[])
{
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    QApplication a(argc, argv);

#if defined(Q_OS_WIN)
        QCoreApplication::setAttribute(Qt::AA_UseSoftwareOpenGL);
#endif

#ifdef Q_OS_LINUX
        QFont ubuntuFont;
        ubuntuFont.setFamily("Ubuntu");
        QApplication::setFont(ubuntuFont);
#endif
    a.setApplicationVersion(APP_VERSION);

#ifdef Q_OS_LINUX
    a.setApplicationName("Sampler-NT1065");
    QCommandLineParser parser;
    parser.setApplicationDescription("NT1065.2 USB3 Stream GUI");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("device", "Device file to open (default - /dev/ntlcyp1)");

    // Process the actual command line arguments given by the user
    parser.process(a);

    const QStringList args = parser.positionalArguments();
    // source is args.at(0), destination is args.at(1)
    if (args.length() > 0)
        device = args.at(0);
#endif

    MainWindow w;
    w.show();

    return a.exec();
}
