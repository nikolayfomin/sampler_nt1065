#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    qRegisterMetaType< cy3device_err_t >("cy3device_err_t");
    qRegisterMetaType< QVector<unsigned short> >("QVector<unsigned short>");

    filedumping = false;

    spectrumform = new SpectrumForm();

    Proc = new DataProcessor();
    ProcThread = new QThread;

    Device = new cy3device("SlaveFifoSync.img");
    DeviceThread = new QThread ;

    ui->setupUi(this);

    connect(ui->checkFillCalc, SIGNAL(clicked(bool)),
            Proc, SLOT(enableFillCalc(bool)));

    connect(ui->checkFFTCalc, SIGNAL(clicked(bool)),
            this, SLOT(FFTCalcSetup()));
    connect(ui->spinSampleRate, SIGNAL(valueChanged(double)),
            this, SLOT(FFTCalcSetup()));
    connect(ui->checkChannel1, SIGNAL(clicked(bool)),
            this, SLOT(FFTCalcSetup()));
    connect(ui->checkChannel2, SIGNAL(clicked(bool)),
            this, SLOT(FFTCalcSetup()));
    connect(ui->checkChannel3, SIGNAL(clicked(bool)),
            this, SLOT(FFTCalcSetup()));
    connect(ui->checkChannel4, SIGNAL(clicked(bool)),
            this, SLOT(FFTCalcSetup()));
    connect(ui->spinAverageFactor,SIGNAL(valueChanged(int)),
            this, SLOT(FFTCalcSetup()));
    connect(ui->spinFrameSkip,SIGNAL(valueChanged(int)),
            this, SLOT(FFTCalcSetup()));

    connect(Proc, SIGNAL(AbortDump()),
            this, SLOT(handleAbortDump()));

    connect(Device, SIGNAL(RawData(QVector<unsigned char>*)),
            Proc, SLOT(ProcessData(QVector<unsigned char>*)));

    connect(Device, SIGNAL(DebugMessage(QString)),
            this, SLOT(DebugParser(QString)));

    connect(Device, SIGNAL(ReportBandwidth(int)),
            this, SLOT(DisplayBandwidth(int)));

    connect(Proc, SIGNAL(ProcessorMessage(QString)),
            this, SLOT(DebugParser(QString)));

    connect(Proc, SIGNAL(FFTData(QVector<double>*,int)),
            spectrumform, SLOT(ProcessData(QVector<double>*,int)));

    Device->moveToThread(DeviceThread);

    DeviceThread->start();

    Proc->moveToThread(ProcThread);
    ProcThread->start();

    this->setWindowTitle(QString("Sampler NT1065 demo v%1").arg(APP_VERSION));
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

    delete spectrumform;
}

void MainWindow::DebugParser(QString Message)
{
    ui->listDebug->addItem(Message);
    ui->listDebug->scrollToBottom();
}

double averBW = 0.0;

void MainWindow::DisplayBandwidth(int BW)
{
#if 0
    // div by 2 cause samples are 16bit
    if (averBW > 1.0)
        averBW += (BW/2.0/1000000.0  - averBW)/2.0;
    else
        averBW = BW/2.0/1000000.0;
#else
    // do not div by 2 cause samples are 8bit
    if (averBW > 1.0)
        averBW += (BW/1000000.0  - averBW)/2.0;
    else
        averBW = BW/1000000.0;
#endif
    ui->labelBW->setText(QString("Bandwidth: ~%1 MSps").arg(averBW,0,'f',1));
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

    DebugParser(QString(""));
    DebugParser(QString("OpenDevice returned %1").arg(cy3device_get_error_string(retval)));

    if (retval == CY3DEV_OK)
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
    if (retval != CY3DEV_OK)
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
    if (retval != CY3DEV_OK)
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

void MainWindow::on_buttonShowSpectrum_clicked()
{
    spectrumform->setVisible(true);
}

void MainWindow::FFTCalcSetup()
{
    //void enableFFTCalc(bool Enable, int SkipFrames, bool Ch1, bool Ch2, bool Ch3, bool Ch4);
    QMetaObject::invokeMethod(Proc ,
                              "enableFFTCalc" ,
                              Qt::BlockingQueuedConnection,
                              Q_ARG(bool, ui->checkFFTCalc->isChecked()),
                              Q_ARG(int, ui->spinFrameSkip->value()),
                              Q_ARG(bool, ui->checkChannel1->isChecked()),
                              Q_ARG(bool, ui->checkChannel2->isChecked()),
                              Q_ARG(bool, ui->checkChannel3->isChecked()),
                              Q_ARG(bool, ui->checkChannel4->isChecked())
                              );

    spectrumform->AverageFactor = ui->spinAverageFactor->value();

    spectrumform->SetupChannels(ui->spinSampleRate->value(),
                                ui->checkChannel1->isChecked(),
                                ui->checkChannel2->isChecked(),
                                ui->checkChannel3->isChecked(),
                                ui->checkChannel4->isChecked()
                                );
}
