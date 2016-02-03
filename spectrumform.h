#ifndef SPECTRUMFORM_H
#define SPECTRUMFORM_H

#include <QWidget>
#include "qcustomplot\QCustomPlot.h"

namespace Ui {
class SpectrumForm;
}

class SpectrumForm : public QWidget
{
    Q_OBJECT

public:
    explicit SpectrumForm(QWidget *parent = 0);
    ~SpectrumForm();

    int AverageFactor;

private:
    Ui::SpectrumForm *ui;
    QCustomPlot* plot;

    QVector<double> *x_axis;
    QVector<double> *y_axis[4];

    const int PLOT_RESOLUTION = 2650; // FFT_SAMPLES/2

public slots:
    void ProcessData(QVector<double> *data, int channel);

    void SetupChannels(bool Ch1, bool Ch2, bool Ch3, bool Ch4);
};

#endif // SPECTRUMFORM_H
