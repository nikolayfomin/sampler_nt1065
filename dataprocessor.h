#ifndef DATA_PROCESSOR_H
#define DATA_PROCESSOR_H

#include <QObject>
#include <QtCore>
#include <QDebug>
#include <QString>
#include <QVector>
#include "fft/fft.h"
#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include "cy3device.h"

class DataProcessor : public QObject
{
    Q_OBJECT

private:
    // dataframe size
    int sample_count;

    QVector<unsigned char> data_pack;

    // filling calculation variables
    int fill_sample_count;
    int cnt_ch1[16];
    int cnt_ch2[16];
    int cnt_ch3[4];
    int cnt_ch4[4];

    bool enFillCalc;
    void FillCalc();

    // file dump variables
    QFile *dumpfile;
    QString dumpfilename;
    long long dump_limit;
    long long dump_count;

    bool enFileDump;
    void FileDump();

    cy3device *device;

    // fft calculation variables
    complex *fft_in;
    complex *fft_out;
    QVector<double>* fft_samples[4];
    int fft_skipframes;
    int fft_adc;
    bool fft_ChEn[4];
    int fftw_cnt;
    QVector<double> fft_window;

    QMutex mutex;

    bool enFFT;
    void FFTCalc();
public:
    explicit DataProcessor(cy3device *dev, QObject *parent = 0);
    ~DataProcessor();
signals:
    void ProcessorMessage(QString Message);

    void FFTData(QVector<double> *data, int channel);

    void AbortDump();

public slots:
    void ProcessData(QVector<unsigned char>* qdata);

    void enableFillCalc(bool Enable, int adc);
    void enableFileDump(bool Enable, QString FileName, long SampleCount);
    void enableFFTCalc(bool Enable, int SkipFrames, bool Ch1, bool Ch2, bool Ch3, bool Ch4, int adc);
};

#endif // DATA_PROCESSOR_H
