#ifndef CY3DEVICE_H
#define CY3DEVICE_H

#include <QtCore>
#include <QDebug>
#include <QObject>
#include <QString>
#include <QVector>
#include <windows.h>
#include <vector>
#include "cyapi\inc\CyAPI.h"

#define MAX_QUEUE_SZ        64
#define MAX_TRANSFER_LENGTH ( 0x400000 )
#define VENDOR_ID           ( 0x04B4 )
#define PRODUCT_STREAM      ( 0x00F1 )
#define PRODUCT_BOOT        ( 0x00F3 )

enum cy3device_err_t {
    CY3DEV_OK = 0,
    CY3DEV_ERR_DRV_NOT_IMPLEMENTED       = -5,
    CY3DEV_ERR_USB_INIT_FAIL             = -10,
    CY3DEV_ERR_NO_DEVICE_FOUND           = -11,
    CY3DEV_ERR_BAD_DEVICE                = -12,
    CY3DEV_ERR_FIRMWARE_FILE_IO_ERROR    = -20,
    CY3DEV_ERR_FIRMWARE_FILE_CORRUPTED   = -21,
    CY3DEV_ERR_ADDFIRMWARE_FILE_IO_ERROR = -25,
    CY3DEV_ERR_REG_WRITE_FAIL            = -32,
    CY3DEV_ERR_FW_TOO_MANY_ERRORS        = -33,
    CY3DEV_ERR_CTRL_TX_FAIL              = -35,
    CY3DEV_ERR_BULK_IO_ERROR             = -37
};

const char* cy3device_get_error_string(cy3device_err_t error);

struct EndPointParams{
    int Attr;
    bool In;
    int MaxPktSize;
    int MaxBurst;
    int Interface;
    int Address;
};

struct DeviceParams{
    CCyFX3Device	*USBDevice;
    CCyUSBEndPoint  *EndPt;
    int				PPX;
    long            TransferSize;
    int				QueueSize;
    int				TimeOut;
    bool			bHighSpeedDevice;
    bool			bSuperSpeedDevice;
    bool			RunStream;
    EndPointParams  CurEndPoint;
};

class cy3device : public QObject
{
    Q_OBJECT

private:
    QString FWName;

    DeviceParams Params;
    std::vector<EndPointParams> Endpoints;

    QTime time;

    int BytesXferred;
    unsigned long Successes;
    unsigned long Failures;

    PUCHAR			*buffers;
    CCyIsoPktInfo	**isoPktInfos;
    PUCHAR			*contexts;
    OVERLAPPED		inOvLap[MAX_QUEUE_SZ];

    int CurrQueue;

    QVector<unsigned char> qdata;

    cy3device_err_t scan(int &loadable_count , int &streamable_count);
    cy3device_err_t prepareEndPoints();
    void getEndPointParamsByInd(unsigned int EndPointInd, int *Attr, bool *In, int *MaxPktSize, int *MaxBurst, int *Interface, int *Address);

    cy3device_err_t startTransfer(unsigned int EndPointInd, int PPX, int QueueSize, int TimeOut);
    Q_INVOKABLE void transfer();
    void abortTransfer(int pending, PUCHAR *buffers, CCyIsoPktInfo **isoPktInfos, PUCHAR *contexts, OVERLAPPED *inOvLap);
    Q_INVOKABLE void stopTransfer();

    void processData(char* data, int size);
public:
    explicit cy3device(const char* firmwareFileName, QObject *parent = 0);

    bool isStreaming;

signals:
    void DebugMessage(QString Message);

    void ReportBandwidth(double BW);

    void RawData(QVector<unsigned char>* qdata);

public slots:
    cy3device_err_t OpenDevice();
    void CloseDevice();

    cy3device_err_t WriteSPI(unsigned char Address, unsigned char Data);
    cy3device_err_t ReadSPI(unsigned char Address, unsigned char *Data);

    void StartStream();
    void StopStream();
};

#endif // CY3DEVICE_H
