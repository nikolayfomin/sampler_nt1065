#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    qRegisterMetaType< cy3device_err_t >("cy3device_err_t");

    Device = new cy3device("SlaveFifoSync.img");
    DeviceThread = new QThread ;

    ui->setupUi(this);

   /* connect(ui->buttonOpenDevice,SIGNAL(clicked(bool)),
            Device,SLOT(OpenDevice()));

    connect(ui->buttonCloseDevice,SIGNAL(clicked(bool)),
            Device,SLOT(CloseDevice()));*/

    Device->moveToThread(DeviceThread);

    DeviceThread->start();
}

MainWindow::~MainWindow()
{
    delete ui;

    if (Device->isStreaming)
        QMetaObject::invokeMethod(Device ,
                                  "StopStream" ,
                                  Qt::BlockingQueuedConnection);

    while (Device->isStreaming)
        Sleep(1);

    QMetaObject::invokeMethod(Device ,
                              "CloseDevice" ,
                              Qt::BlockingQueuedConnection);

    // выключаем поток и крутим цикл пока он не завершится
    DeviceThread->quit();
    while(DeviceThread->isRunning()) ;

    delete Device;
    delete DeviceThread;
}

void MainWindow::DebugParser(QString Message)
{
    ui->listDebug->addItem(Message);
}

void MainWindow::on_buttonOpenDevice_clicked()
{
    cy3device_err_t retval;

    QMetaObject::invokeMethod(Device ,
                              "OpenDevice" ,
                              Qt::BlockingQueuedConnection ,
                              Q_RETURN_ARG( cy3device_err_t , retval));

    DebugParser(QString("OpenDevice returned %1").arg(cy3device_get_error_string(retval)));

    if (retval == FX3_ERR_OK)
    {
        ui->buttonOpenDevice->setEnabled(false);
        ui->buttonCloseDevice->setEnabled(true);
        ui->buttonReadID->setEnabled(true);
        ui->buttonStartStream->setEnabled(true);
        ui->buttonStopStream->setEnabled(false);
    }
}

void MainWindow::on_buttonCloseDevice_clicked()
{
    if (Device->isStreaming)
        QMetaObject::invokeMethod(Device ,
                                  "StopStream" ,
                                  Qt::BlockingQueuedConnection);

    while (Device->isStreaming)
        Sleep(1);

    QMetaObject::invokeMethod(Device ,
                              "CloseDevice" ,
                              Qt::BlockingQueuedConnection);

    DebugParser(QString("CloseDevice returned"));

    ui->buttonOpenDevice->setEnabled(true);
    ui->buttonCloseDevice->setEnabled(false);
    ui->buttonReadID->setEnabled(false);
    ui->buttonStartStream->setEnabled(false);
    ui->buttonStopStream->setEnabled(false);
}

void MainWindow::on_buttonReadID_clicked()
{
    unsigned char data[2];
    cy3device_err_t retval;

    QMetaObject::invokeMethod(Device ,
                              "ReadSPI" ,
                              Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG( cy3device_err_t , retval),
                              Q_ARG(unsigned char, 0x00),
                              Q_ARG(unsigned char*, &data[0]));
    if (retval != FX3_ERR_OK)
    {
        DebugParser(QString("ReadSPI returned %1").arg(cy3device_get_error_string(retval)));
        return;
    }

    QMetaObject::invokeMethod(Device ,
                              "ReadSPI" ,
                              Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG( cy3device_err_t , retval),
                              Q_ARG(unsigned char, 0x01),
                              Q_ARG(unsigned char*, &data[1]));
    if (retval != FX3_ERR_OK)
    {
        DebugParser(QString("ReadSPI returned %1").arg(cy3device_get_error_string(retval)));
        return;
    }

    int ID = (data[0] << 5) | (data[1] >> 3);
    int Rev = data[1] & 0x07;

    DebugParser(QString("ID: %1, Rev: %2").arg(ID).arg(Rev));
}

void MainWindow::on_buttonStartStream_clicked()
{
    QMetaObject::invokeMethod(Device ,
                              "StartStream" ,
                              Qt::BlockingQueuedConnection);

    ui->buttonStartStream->setEnabled(false);
    ui->buttonStopStream->setEnabled(true);

    DebugParser(QString("Streaming started"));
}

void MainWindow::on_buttonStopStream_clicked()
{
    QMetaObject::invokeMethod(Device ,
                              "StopStream" ,
                              Qt::BlockingQueuedConnection);

    while(Device->isStreaming)
        Sleep(1);

    ui->buttonStartStream->setEnabled(true);
    ui->buttonStopStream->setEnabled(false);

    DebugParser(QString("Streaming stopped"));
}
