#ifndef DATA_PROCESSOR_H
#define DATA_PROCESSOR_H

#include <QObject>
#include <QtCore>
#include <QDebug>
#include <QString>
#include <QVector>
#include <windows.h>
#include "fftw/inc/fftw3.h"

class DataProcessor : public QObject
{
    Q_OBJECT

private:
    const UINT64 MAX_FILL_SAMPLES = 16*1024*1024;

    const int FFT_SAMPLES_PER_FRAME = 5300;
    const int FFT_SKIP_FRAMES = 100;

    int cnt_ch1[4];
    int cnt_ch2[4];
    int cnt_ch3[4];
    int cnt_ch4[4];
    UINT64 sample_count;

    QFile *dumpfile;
    QString dumpfilename;
    long dump_limit;
    long dump_count;

    float* fftw_in;
    fftwf_complex* fftw_out;
    fftwf_plan fftw_pl;
    QVector<double>* fft_samples[4];
    int fft_skipframes;
    bool fft_ChEn[4];
    int fftw_cnt;

    bool enFillCalc;
    void FillCalc(QVector<unsigned short> qdata);

    bool enFileDump;
    void FileDump(QVector<unsigned short> qdata);

    bool enFFT;
    void FFTCalc(QVector<unsigned short> qdata);
public:
    explicit DataProcessor(QObject *parent = 0);
    ~DataProcessor();
signals:
    void ProcessorMessage(QString Message);

    void FFTData(QVector<double> *data, int channel);

    void AbortDump();

public slots:
    void ProcessData(QVector<unsigned short> qdata);

    void enableFillCalc(bool Enable);
    void enableFileDump(bool Enable, QString FileName, long SampleCount);
    void enableFFTCalc(bool Enable, int SkipFrames, bool Ch1, bool Ch2, bool Ch3, bool Ch4);
};

#endif // DATA_PROCESSOR_H
