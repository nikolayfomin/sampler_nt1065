#ifndef CY3DEVICE_H
#define CY3DEVICE_H

#include <QtCore>
#include <QObject>
#include <QString>
#include <windows.h>
#include <vector>
#include "library\inc\CyAPI.h"

#define MAX_QUEUE_SZ        64
#define MAX_TRANSFER_LENGTH ( 0x400000 )
#define VENDOR_ID           ( 0x04B4 )
#define PRODUCT_STREAM      ( 0x00F1 )
#define PRODUCT_BOOT        ( 0x00F3 )

enum cy3device_err_t {
    FX3_ERR_OK = 0,
    FX3_ERR_DRV_NOT_IMPLEMENTED       = -5,
    FX3_ERR_USB_INIT_FAIL             = -10,
    FX3_ERR_NO_DEVICE_FOUND           = -11,
    FX3_ERR_BAD_DEVICE                = -12,
    FX3_ERR_FIRMWARE_FILE_IO_ERROR    = -20,
    FX3_ERR_FIRMWARE_FILE_CORRUPTED   = -21,
    FX3_ERR_ADDFIRMWARE_FILE_IO_ERROR = -25,
    FX3_ERR_REG_WRITE_FAIL            = -32,
    FX3_ERR_FW_TOO_MANY_ERRORS        = -33,
    FX3_ERR_CTRL_TX_FAIL              = -35
};

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
    bool            isStreaming;
    EndPointParams  CurEndPoint;
};

class cy3device : public QObject
{
    Q_OBJECT

private:
    QString FWName;

    DeviceParams Device;
    std::vector<EndPointParams> Endpoints;


    UINT64 BytesXferred;
    unsigned long Successes;
    unsigned long Failures;

    PUCHAR			*buffers;
    CCyIsoPktInfo	**isoPktInfos;
    PUCHAR			*contexts;
    OVERLAPPED		inOvLap[MAX_QUEUE_SZ];

    int CurrQueue;

    cy3device_err_t scan(int &loadable_count , int &streamable_count);
    cy3device_err_t prepareEndPoints();
    void getEndPointParamsByInd(unsigned int EndPointInd, int *Attr, bool *In, int *MaxPktSize, int *MaxBurst, int *Interface, int *Address);

    void startStream(unsigned int EndPointInd, int PPX, int QueueSize, int TimeOut);
    void transfer();
    void abortTransfer(int pending, PUCHAR *buffers, CCyIsoPktInfo **isoPktInfos, PUCHAR *contexts, OVERLAPPED *inOvLap);
    void stopStream();

    void processData(char* data, int size);
public:
    explicit cy3device(const char* firmwareFileName, QObject *parent = 0);

    cy3device_err_t OpenDevice();
    void CloseDevice();

    void WriteSPI(unsigned char Address, unsigned char Data);
    unsigned char ReadSPI(unsigned char Address);

    /*int ReviewDevices();
    int LoadRAM(const char* fwFileName);
    int GetStreamerDevice();
    int Read16bitSPI(unsigned char addr, unsigned char* data);
    int Send16bitSPI(unsigned char addr, unsigned char data);*/

signals:

public slots:
};

#endif // CY3DEVICE_H
