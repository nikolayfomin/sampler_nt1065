#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    qRegisterMetaType< cy3device_err_t >("cy3device_err_t");
    qRegisterMetaType< QVector<unsigned short> >("QVector<unsigned short>");

    filedumping = false;

    Proc = new DataProcessor();
    ProcThread = new QThread;

    Device = new cy3device("SlaveFifoSync.img");
    DeviceThread = new QThread ;

    ui->setupUi(this);

    connect(ui->checkFillCalc, SIGNAL(clicked(bool)),
            Proc, SLOT(enableFillCalc(bool)));

    connect(Proc, SIGNAL(AbortDump()),
            this, SLOT(handleAbortDump()));

    connect(Device, SIGNAL(RawData(QVector<unsigned short>)),
            Proc, SLOT(ProcessData(QVector<unsigned short>)));

    connect(Proc, SIGNAL(ProcessorMessage(QString)),
            this, SLOT(DebugParser(QString)));

    Device->moveToThread(DeviceThread);

    DeviceThread->start();

    Proc->moveToThread(ProcThread);
    ProcThread->start();
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

    // выключаем поток и крутим цикл пока он не завершится
    ProcThread->quit();
    while(ProcThread->isRunning()) ;

    delete Device;
    delete DeviceThread;

    delete Proc;
    delete ProcThread;
}

void MainWindow::DebugParser(QString Message)
{
    ui->listDebug->addItem(Message);
    ui->listDebug->scrollToBottom();
}

void MainWindow::handleAbortDump()
{
    filedumping = false;

    ui->buttonFileDump->setText("Dump data to file");
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

void MainWindow::on_buttonFileDump_clicked()
{
    if (filedumping)
    {
        filedumping = false;

        QMetaObject::invokeMethod(Proc ,
                                  "enableFileDump" ,
                                  Qt::BlockingQueuedConnection,
                                  Q_ARG(bool, false),
                                  Q_ARG(QString, ui->editDumpFileName->text()),
                                  Q_ARG(long, 0));
        ui->buttonFileDump->setText("Dump data to file");
    }
    else
    {
        filedumping = true;

        QMetaObject::invokeMethod(Proc ,
                                  "enableFileDump" ,
                                  Qt::BlockingQueuedConnection,
                                  Q_ARG(bool, true),
                                  Q_ARG(QString, ui->editDumpFileName->text()),
                                  Q_ARG(long, ui->spinFileSize->value()));
        ui->buttonFileDump->setText("Stop dumping");
    }
}

void MainWindow::on_buttonSetFileName_clicked()
{
    QString fileName;
    fileName = QFileDialog::getSaveFileName(this,
        tr("Select file for signal dump"),
        "",
        tr("Binary files (*.bin);;All files (*.*)" )
    );
    if ( fileName.size() > 1 ) {
        ui->editDumpFileName->setText( fileName );
    }
}
