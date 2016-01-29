#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "cy3device.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_buttonOpenDevice_clicked();
    void DebugParser(QString Message);

    void on_buttonCloseDevice_clicked();

    void on_buttonReadID_clicked();

private:
    Ui::MainWindow *ui;

    cy3device *Device;
    QThread *DeviceThread;
};

#endif // MAINWINDOW_H
