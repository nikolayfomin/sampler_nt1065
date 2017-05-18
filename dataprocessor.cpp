#include "dataprocessor.h"

const int MAX_SAMPLES = 2*1024*1024;

const int FFT_SAMPLES_PER_FRAME = 65536;
const int FFT_SKIP_FRAMES = 5;

//const double decode_samples[4] = {1.0, 3.0, -1.0, -3.0};
const double decode_samples[4] = {1.0, -1.0, 3.0, -3.0};


DataProcessor::DataProcessor(cy3device *dev, QObject *parent) : QObject(parent)
{
    data_pack.resize(2*MAX_SAMPLES);
    sample_count = 0;

    fill_sample_count = 0;
    std::fill( cnt_ch1, cnt_ch1 + 4, 0 );
    std::fill( cnt_ch2, cnt_ch2 + 4, 0 );
    std::fill( cnt_ch3, cnt_ch3 + 4, 0 );
    std::fill( cnt_ch4, cnt_ch4 + 4, 0 );

    dump_count = 0;
    dumpfile = NULL;
    dump_limit = 0;

    device = dev;

    fft_in  = new complex[FFT_SAMPLES_PER_FRAME];
    fft_out = new complex[FFT_SAMPLES_PER_FRAME];

    fftw_cnt = FFT_SKIP_FRAMES;
    fft_skipframes = FFT_SKIP_FRAMES;
    fft_samples[0] = new QVector<double>(FFT_SAMPLES_PER_FRAME/2, 0.0);
    fft_samples[1] = new QVector<double>(FFT_SAMPLES_PER_FRAME/2, 0.0);
    fft_samples[2] = new QVector<double>(FFT_SAMPLES_PER_FRAME/2, 0.0);
    fft_samples[3] = new QVector<double>(FFT_SAMPLES_PER_FRAME/2, 0.0);
    fft_ChEn[0] = fft_ChEn[1] = fft_ChEn[2] = fft_ChEn[3] = false;

    fft_window.resize(FFT_SAMPLES_PER_FRAME);

    for (int i = 0; i < FFT_SAMPLES_PER_FRAME; i++)
        fft_window[i] = 0.5 - 0.5*cos(2*M_PI*i/(FFT_SAMPLES_PER_FRAME - 1));

    enFillCalc = 0;
    enFileDump = 0;
    enFFT = 0;
}

DataProcessor::~DataProcessor()
{
    if (dumpfile)
    {
        if (dumpfile->isOpen())
            dumpfile->close();

        delete dumpfile;
    }

    delete fft_samples[0];
    delete fft_samples[1];
    delete fft_samples[2];
    delete fft_samples[3];
    delete fft_in;
    delete fft_out;
}

void DataProcessor::FillCalc()
{
    for(int i = 0; i < sample_count; i++)
    {
        cnt_ch4[(data_pack[i] & 0x03) >> 0]++;
        cnt_ch3[(data_pack[i] & 0x0C) >> 2]++;
        cnt_ch2[(data_pack[i] & 0x30) >> 4]++;
        cnt_ch1[(data_pack[i] & 0xC0) >> 6]++;
        fill_sample_count++;
    }

    if (fill_sample_count >= 4*MAX_SAMPLES)
    {
        double fill1[4],fill2[4],fill3[4],fill4[4];
        for (int i = 0; i < 4; i++)
        {
            fill1[i] = 100.0 * cnt_ch1[i]/fill_sample_count;
            fill2[i] = 100.0 * cnt_ch2[i]/fill_sample_count;
            fill3[i] = 100.0 * cnt_ch3[i]/fill_sample_count;
            fill4[i] = 100.0 * cnt_ch4[i]/fill_sample_count;
        }
        //                                          -3        -1        1         3
        qDebug(" ");
        //qDebug("Channel1 fill: %lf %lf %lf %lf", fill1[3], fill1[2], fill1[0], fill1[1]);
        //qDebug("Channel2 fill: %lf %lf %lf %lf", fill2[3], fill2[2], fill2[0], fill2[1]);
        //qDebug("Channel3 fill: %lf %lf %lf %lf", fill3[3], fill3[2], fill3[0], fill3[1]);
        //qDebug("Channel4 fill: %lf %lf %lf %lf", fill4[3], fill4[2], fill4[0], fill4[1]);

        QString msg;
        msg += QTime::currentTime().toString() + ":" + QString("%1").arg(QTime::currentTime().msec(),3, 10, QLatin1Char( '0' )) + "\n";
        msg += QString("Channel1 fill: %1 %2 %3 %4\n").arg(fill1[3]).arg(fill1[1]).arg(fill1[0]).arg(fill1[2]);
        msg += QString("Channel2 fill: %1 %2 %3 %4\n").arg(fill2[3]).arg(fill2[1]).arg(fill2[0]).arg(fill2[2]);
        msg += QString("Channel3 fill: %1 %2 %3 %4\n").arg(fill3[3]).arg(fill3[1]).arg(fill3[0]).arg(fill3[2]);
        msg += QString("Channel4 fill: %1 %2 %3 %4\n").arg(fill4[3]).arg(fill4[1]).arg(fill4[0]).arg(fill4[2]);

        emit ProcessorMessage(msg);

        std::fill( cnt_ch1, cnt_ch1 + 4, 0 );
        std::fill( cnt_ch2, cnt_ch2 + 4, 0 );
        std::fill( cnt_ch3, cnt_ch3 + 4, 0 );
        std::fill( cnt_ch4, cnt_ch4 + 4, 0 );

        fill_sample_count = 0;
    }
}

#define MIN(x,y) ((x)>(y)?(y):(x))

void DataProcessor::FileDump()
{

    mutex.lock();
    if (!enFileDump || !dumpfile->isOpen()) {
        mutex.unlock();
        return;
    }

    long long toWrite;

    if (dump_limit)
        toWrite = MIN(sample_count, dump_limit-dump_count);
    else
        toWrite = sample_count;

    dumpfile->write((const char*)data_pack.data(), toWrite);
    mutex.unlock();

    dumpfile->flush();
    dump_count += toWrite;

    if (dump_limit && dump_count >= dump_limit)
    {
        enableFileDump(false,"",0);

        emit AbortDump();
    }
}

char swap_bits_in_byte(unsigned char in)
{
    int i = 0;
    unsigned char out = 0;

    for (; i < 8; i++) {
        int b = (in & 1);
        out |= b;
        out = (out << 1) & (~0x01);
        in = in >> 1;
    }
    return out;
}

void DataProcessor::FFTCalc()
{
    if (sample_count < FFT_SAMPLES_PER_FRAME)
        return;

    // skip frames so that we don't spam FFT too much
    if (fftw_cnt--)
        return;

    fftw_cnt = fft_skipframes;

// Channel 1
    if (fft_ChEn[0])
    {
        if (fft_adc == 0) {
            for (int i = 0; i < FFT_SAMPLES_PER_FRAME; i++) {
                fft_in[i] = decode_samples[(data_pack[i]&0xC0)>>6] * fft_window[i];
                //qDebug() << (data[i]&0x03) << " " << decode_samples[(data[i]&0x03)>>0];
            }
        } else if (fft_adc == 1) {
            for (int i = 0; i < FFT_SAMPLES_PER_FRAME; i++) {
                unsigned char b = data_pack[i];
                int f = (int)((b&0xf0)>>4);
                if (f > 7) {
                    f = f - 16;
                }
                fft_in[i] = f * fft_window[i];;
              //  qDebug() << b << " ";
            }
        } else if (fft_adc == 2) {
            for (int i = 0; i < FFT_SAMPLES_PER_FRAME; i++) {
                unsigned char b = data_pack[i];
                int f = (int)b;
                if (f > 127) {
                    f = f - 256;
                }
                fft_in[i] = f * fft_window[i];
                //qDebug() << (data[i]&0x03) << " " << decode_samples[(data[i]&0x03)>>0];
            }
        }

        CFFT::Forward(fft_in, fft_out, FFT_SAMPLES_PER_FRAME);
        for (int i = 0; i < FFT_SAMPLES_PER_FRAME/2; i++)
            (*fft_samples[0])[i] = 10.0*log10(fft_out[i].norm());

        emit FFTData(fft_samples[0], 0);
    }

// Channel 2
    if (fft_ChEn[1])
    {
        if (fft_adc == 0) {
            for (int i = 0; i < FFT_SAMPLES_PER_FRAME; i++) {
                fft_in[i] = decode_samples[(data_pack[i]&0x30)>>4] * fft_window[i];
                //qDebug() << (data[i]&0x03) << " " << decode_samples[(data[i]&0x03)>>0];
            }
        } else if (fft_adc == 1) {
            for (int i = 0; i < FFT_SAMPLES_PER_FRAME; i++) {
                unsigned char b = data_pack[i];
                int f = (int)((b&0x0f));
                if (f > 7) {
                    f = f - 16;
                }
                fft_in[i] = f * fft_window[i];;
            }
        }

        CFFT::Forward(fft_in, fft_out, FFT_SAMPLES_PER_FRAME);
        for (int i = 0; i < FFT_SAMPLES_PER_FRAME/2; i++)
            (*fft_samples[1])[i] = 10.0*log10(fft_out[i].norm());

        emit FFTData(fft_samples[1], 1);
    }

// Channel 3
    if (fft_ChEn[2])
    {
        for (int i = 0; i < FFT_SAMPLES_PER_FRAME; i++) {
            fft_in[i] = decode_samples[(data_pack[i]&0x0C)>>2] * fft_window[i];
            //qDebug() << (data[i]&0x03) << " " << decode_samples[(data[i]&0x03)>>0];
        }
        CFFT::Forward(fft_in, fft_out, FFT_SAMPLES_PER_FRAME);
        for (int i = 0; i < FFT_SAMPLES_PER_FRAME/2; i++)
            (*fft_samples[2])[i] = 10.0*log10(fft_out[i].norm());

        emit FFTData(fft_samples[2], 2);
    }

// Channel 4
    if (fft_ChEn[3])
    {
        for (int i = 0; i < FFT_SAMPLES_PER_FRAME; i++) {
            fft_in[i] = decode_samples[(data_pack[i]&0x03)>>0] * fft_window[i];
            //qDebug() << (data[i]&0x03) << " " << decode_samples[(data[i]&0x03)>>0];
        }
        CFFT::Forward(fft_in, fft_out, FFT_SAMPLES_PER_FRAME);
        for (int i = 0; i < FFT_SAMPLES_PER_FRAME/2; i++)
            (*fft_samples[3])[i] = 10.0*log10(fft_out[i].norm());

        emit FFTData(fft_samples[3], 3);
    }
}

void DataProcessor::ProcessData(QVector<unsigned char> *qdata)
{
    if (!qdata)
        return;

    memcpy(data_pack.data() + sample_count, qdata->data(), qdata->size());
    sample_count += qdata->size();

    device->ccInc(qdata->size() * (-1));
    delete qdata;

    if (sample_count < MAX_SAMPLES)
        return;


    if (enFillCalc)
        FillCalc();

    if (enFileDump) {
        FileDump();
    }

    if (enFFT)
        FFTCalc();

    sample_count = 0;
}

void DataProcessor::enableFillCalc(bool Enable)
{
    enFillCalc = Enable;
}

void DataProcessor::enableFileDump(bool Enable, QString FileName, long SampleCount)
{
    if (enFileDump == Enable)
        return;

    mutex.lock();

    enFileDump = Enable;

    if (Enable)
    {
        dump_count = 0;
        dump_limit = (long long)SampleCount * 1024 * 1024;
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
            emit ProcessorMessage(QString("Dumped %1 Mbytes to %2").arg(dump_count/(1024*1024)).arg(dumpfilename));
    }

    mutex.unlock();
}

void DataProcessor::enableFFTCalc(bool Enable, int SkipFrames, bool Ch1, bool Ch2, bool Ch3, bool Ch4, int adc)
{
    enFFT = Enable;

    if (enFFT)
    {
        fft_ChEn[0] = Ch1;
        fft_ChEn[1] = Ch2;
        fft_ChEn[2] = Ch3;
        fft_ChEn[3] = Ch4;

        fft_skipframes = SkipFrames;
        fft_adc = adc;
    }
}
