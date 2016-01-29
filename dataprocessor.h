#ifndef DATA_PROCESSOR_H
#define DATA_PROCESSOR_H

#include <QObject>
#include <QtCore>
#include <QDebug>
#include <QString>
#include <QVector>
#include <windows.h>

class DataProcessor : public QObject
{
    Q_OBJECT

private:
    const UINT64 MAX_SAMPLES = 16*1024*1024;

    int cnt_ch1[4];
    int cnt_ch2[4];
    int cnt_ch3[4];
    int cnt_ch4[4];
    UINT64 sample_count;

    //FILE *dumpfile;
    QFile *dumpfile;
    QString dumpfilename;
    long dump_limit;
    long dump_count;

    bool enFillCalc;
    void FillCalc(QVector<unsigned short> qdata);

    bool enFileDump;
    void FileDump(QVector<unsigned short> qdata);
public:
    explicit DataProcessor(QObject *parent = 0);
    ~DataProcessor();
signals:
    void ProcessorMessage(QString Message);

    void AbortDump();

public slots:
    void ProcessData(QVector<unsigned short> qdata);

    void enableFillCalc(bool Enable);
    void enableFileDump(bool Enable, QString FileName, long SampleCount);
};

#endif // DATA_PROCESSOR_H
