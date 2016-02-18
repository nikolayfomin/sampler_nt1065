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
    // dataframe size
    const int MAX_SAMPLES = 2*1024*1024;
    int sample_count;

    QVector<unsigned short> data_pack;

    // filling calculation variables
    int fill_sample_count;
    int cnt_ch1[4];
    int cnt_ch2[4];
    int cnt_ch3[4];
    int cnt_ch4[4];

    bool enFillCalc;
    void FillCalc();

    // file dump variables
    QFile *dumpfile;
    QString dumpfilename;
    long dump_limit;
    long dump_count;

    bool enFileDump;
    void FileDump();

    // fft calculation variables
    const int FFT_SAMPLES_PER_FRAME = 65536;
    const int FFT_SKIP_FRAMES = 5;
    float* fftw_in;
    fftwf_complex* fftw_out;
    fftwf_plan fftw_pl;
    QVector<double>* fft_samples[4];
    int fft_skipframes;
    bool fft_ChEn[4];
    int fftw_cnt;
    QVector<double> fft_window;

    bool enFFT;
    void FFTCalc();
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
