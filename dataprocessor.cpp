#include "dataprocessor.h"

DataProcessor::DataProcessor(QObject *parent) : QObject(parent)
{
    std::fill( cnt_ch1, cnt_ch1 + 4, 0 );
    std::fill( cnt_ch2, cnt_ch2 + 4, 0 );
    std::fill( cnt_ch3, cnt_ch3 + 4, 0 );
    std::fill( cnt_ch4, cnt_ch4 + 4, 0 );
    sample_count = 0;
    dump_count = 0;

    dumpfile = NULL;
    dump_limit = 0;

    enFillCalc = 0;
    enFileDump = 0;
}

DataProcessor::~DataProcessor()
{
    if (dumpfile)
    {
        if (dumpfile->isOpen())
            dumpfile->close();

        delete dumpfile;
    }
}

void DataProcessor::FillCalc(QVector<unsigned short> qdata)
{
    foreach(unsigned short data, qdata)
    {
        sample_count++;
        cnt_ch1[(data & 0x03) >> 0]++;
        cnt_ch2[(data & 0x0C) >> 2]++;
        cnt_ch3[(data & 0x30) >> 4]++;
        cnt_ch4[(data & 0xC0) >> 6]++;
    }

    if (sample_count > MAX_SAMPLES)
    {
        double fill1[4],fill2[4],fill3[4],fill4[4];
        for (int i = 0; i < 4; i++)
        {
            fill1[i] = 100.0 * cnt_ch1[i]/sample_count;
            fill2[i] = 100.0 * cnt_ch2[i]/sample_count;
            fill3[i] = 100.0 * cnt_ch3[i]/sample_count;
            fill4[i] = 100.0 * cnt_ch4[i]/sample_count;
        }
        //                                          -3        -1        1         3
        qDebug(" ");
        //qDebug("Channel1 fill: %lf %lf %lf %lf", fill1[3], fill1[2], fill1[0], fill1[1]);
        //qDebug("Channel2 fill: %lf %lf %lf %lf", fill2[3], fill2[2], fill2[0], fill2[1]);
        //qDebug("Channel3 fill: %lf %lf %lf %lf", fill3[3], fill3[2], fill3[0], fill3[1]);
        //qDebug("Channel4 fill: %lf %lf %lf %lf", fill4[3], fill4[2], fill4[0], fill4[1]);

        QString msg;
        msg += QTime::currentTime().toString() + ":" + QString("%1").arg(QTime::currentTime().msec(),3, 10, QLatin1Char( '0' )) + "\n";
        msg += QString("Channel1 fill: %1 %2 %3 %4\n").arg(fill1[3]).arg(fill1[2]).arg(fill1[0]).arg(fill1[1]);
        msg += QString("Channel2 fill: %1 %2 %3 %4\n").arg(fill2[3]).arg(fill2[2]).arg(fill2[0]).arg(fill2[1]);
        msg += QString("Channel3 fill: %1 %2 %3 %4\n").arg(fill3[3]).arg(fill3[2]).arg(fill3[0]).arg(fill3[1]);
        msg += QString("Channel4 fill: %1 %2 %3 %4\n").arg(fill4[3]).arg(fill4[2]).arg(fill4[0]).arg(fill4[1]);

        qDebug() << msg;

        emit ProcessorMessage(msg);

        std::fill( cnt_ch1, cnt_ch1 + 4, 0 );
        std::fill( cnt_ch2, cnt_ch2 + 4, 0 );
        std::fill( cnt_ch3, cnt_ch3 + 4, 0 );
        std::fill( cnt_ch4, cnt_ch4 + 4, 0 );
        sample_count = 0;
    }
}

void DataProcessor::FileDump(QVector<unsigned short> qdata)
{
    if (!enFileDump || !dumpfile->isOpen())
        return;

    foreach(unsigned short data, qdata)
    {
        if (dump_limit && dump_count >= dump_limit)
        {
            enableFileDump(false,"",0);

            emit AbortDump();

            return;
        }
        unsigned char cdata = data & 0xFF;
        //fputc(cdata, dumpfile);
        dumpfile->write((const char*)&cdata,1);
        dump_count++;
    }
}

void DataProcessor::ProcessData(QVector<unsigned short> qdata)
{
    if (enFillCalc)
        FillCalc(qdata);

    if (enFileDump)
        FileDump(qdata);
}

void DataProcessor::enableFillCalc(bool Enable)
{
    enFillCalc = Enable;
}

void DataProcessor::enableFileDump(bool Enable, QString FileName, long SampleCount)
{
    if (enFileDump == Enable)
        return;

    enFileDump = Enable;

    if (Enable)
    {
        dump_count = 0;
        dump_limit = SampleCount;
        dumpfilename = FileName;

        dumpfile = new QFile(dumpfilename);
        dumpfile->remove();

        dumpfile->open(QIODevice::Append);

        emit ProcessorMessage(QString("Started dumping to %1").arg(FileName));
    }
    else
    {
        dumpfile->close();
        delete dumpfile;
        dumpfile = NULL;

        if (dump_count)
            emit ProcessorMessage(QString("Dumped %1 bytes to %2").arg(dump_count).arg(dumpfilename));
    }
}
