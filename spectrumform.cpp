#include "spectrumform.h"
#include "ui_spectrumform.h"

const int PLOT_RESOLUTION = 32768; // FFT_SAMPLES/2
// default sampling rate is 53MHz, fs/2 = 26.5MHz
const double INIT_RES_TO_FREQ = 26.5/PLOT_RESOLUTION;

SpectrumForm::SpectrumForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SpectrumForm)
{
    ui->setupUi(this);

    plot = ui->plotSpectrum;

    AverageFactor = 10;

    plot->addGraph();
    plot->graph(0)->setPen(QPen(Qt::blue));
    plot->addGraph();
    plot->graph(1)->setPen(QPen(Qt::red));
    plot->addGraph();
    plot->graph(2)->setPen(QPen(Qt::black));
    plot->addGraph();
    plot->graph(3)->setPen(QPen(Qt::green));

    plot->setInteraction(QCP::iRangeDrag, true);
    plot->axisRect()->setRangeDrag(Qt::Horizontal);
    plot->setInteraction(QCP::iRangeZoom, true);
    plot->axisRect()->setRangeZoom(Qt::Horizontal);

    x_axis = new QVector<double>(PLOT_RESOLUTION);
    for (int i = 0; i < PLOT_RESOLUTION; i++)
        (*x_axis)[i] = (double)i*INIT_RES_TO_FREQ; // in MHz

    y_axis[0] = new QVector<double>(PLOT_RESOLUTION, 0.0);
    y_axis[1] = new QVector<double>(PLOT_RESOLUTION, 0.0);
    y_axis[2] = new QVector<double>(PLOT_RESOLUTION, 0.0);
    y_axis[3] = new QVector<double>(PLOT_RESOLUTION, 0.0);

    plot->xAxis->setRange(0, PLOT_RESOLUTION*INIT_RES_TO_FREQ); // in MHz
    plot->yAxis->setRange(0, 100);

    plot->xAxis->setLabel("Frequency, MHz");
    plot->yAxis->setLabel("Amplitude");
}

SpectrumForm::~SpectrumForm()
{
    delete x_axis;

    delete y_axis[0];
    delete y_axis[1];
    delete y_axis[2];
    delete y_axis[3];

    delete ui;
}

void SpectrumForm::ProcessData(QVector<double> *data, int channel)
{
     if (data->size() < PLOT_RESOLUTION)
         return;

     for (int i = 0; i < PLOT_RESOLUTION; i++)
     {
         //(*expAved)[i] -= (*expAved)[i]*expAveC;
         //(*expAved)[i] += (*spc)[i]*expAveC;
         (*y_axis[channel])[i] += ((*data)[i] - (*y_axis[channel])[i])/AverageFactor;
     }

     plot->graph(channel)->setData(*x_axis, *y_axis[channel]);

     plot->replot();
}

void SpectrumForm::SetupChannels(double SampleRate, bool Ch1, bool Ch2, bool Ch3, bool Ch4)
{
    plot->graph(0)->setVisible(Ch1);
    plot->graph(1)->setVisible(Ch2);
    plot->graph(2)->setVisible(Ch3);
    plot->graph(3)->setVisible(Ch4);

    for (int i = 0; i < PLOT_RESOLUTION; i++)
        (*x_axis)[i] = i * (SampleRate/2.0)/PLOT_RESOLUTION; // in MHz

    plot->xAxis->setRange(0, SampleRate/2.0); // in MHz

    plot->replot();
}
