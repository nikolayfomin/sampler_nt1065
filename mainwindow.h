#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "cy3device.h"
#include "dataprocessor.h"
#include "spectrumform.h"
#include <QFileDialog>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

private:
    bool filedumping;
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void closeEvent(QCloseEvent *event);

    SpectrumForm *spectrumform;

private slots:
    void on_buttonOpenDevice_clicked();
    void DebugParser(QString Message);
    void DisplayBandwidth(int BW);

    void handleAbortDump();

    void on_buttonCloseDevice_clicked();

    void on_buttonReadID_clicked();

    void on_buttonStartStream_clicked();

    void on_buttonStopStream_clicked();

    void on_buttonFileDump_clicked();

    void on_buttonSetFileName_clicked();

    void on_buttonShowSpectrum_clicked();

    void FFTCalcSetup();

    void on_comboBoxADC_currentIndexChanged(int index);

private:
    Ui::MainWindow *ui;

    cy3device *Device;
    QThread *DeviceThread;

    DataProcessor *Proc;
    QThread *ProcThread;
};

#endif // MAINWINDOW_H
