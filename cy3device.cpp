#include "cy3device.h"

cy3device::cy3device(const char* firmwareFileName, QObject *parent) : QObject(parent)
{
    FWName = firmwareFileName;

    BytesXferred = 0;
    Successes = 0;
    Failures = 0;

    CurrQueue = 0;
}

cy3device_err_t cy3device::OpenDevice()
{
    Device.USBDevice = new CCyFX3Device();

    Device.isStreaming = false;
    Device.RunStream = false;

    int boot = 0;
    int stream = 0;
    cy3device_err_t res = scan( boot, stream );
    if ( res != FX3_ERR_OK )
        return res;

    bool need_fw_load = stream == 0 && boot > 0;

    if ( need_fw_load )
    {
        fprintf( stderr, "cy3device::init() fw load needed\n" );

        QFileInfo checkFile(FWName);

        if (!(checkFile.exists() && checkFile.isFile()))
            return FX3_ERR_FIRMWARE_FILE_IO_ERROR;

        if ( Device.USBDevice->IsBootLoaderRunning() )
        {
            int retCode  = Device.USBDevice->DownloadFw((char*)FWName.toStdString().c_str(), FX3_FWDWNLOAD_MEDIA_TYPE::RAM);

            if ( retCode != FX3_FWDWNLOAD_ERROR_CODE::SUCCESS )
            {
                switch( retCode )
                {
                    case INVALID_FILE:
                    case CORRUPT_FIRMWARE_IMAGE_FILE:
                    case INCORRECT_IMAGE_LENGTH:
                    case INVALID_FWSIGNATURE:
                        return FX3_ERR_FIRMWARE_FILE_CORRUPTED;
                    default:
                        return FX3_ERR_CTRL_TX_FAIL;
                }
            } else
                fprintf( stderr, "cy3device::init() boot ok!\n" );

        } else
        {
            fprintf( stderr, "__error__ cy3device::init() StartParams.USBDevice->IsBootLoaderRunning() is FALSE\n" );
            return FX3_ERR_BAD_DEVICE;
        }
    }

    if ( need_fw_load )
    {
        int PAUSE_AFTER_FLASH_SECONDS = 2;
        fprintf( stderr, "cy3device::Init() flash completed!\nPlease wait for %d seconds\n", PAUSE_AFTER_FLASH_SECONDS );
        for ( int i = 0; i < PAUSE_AFTER_FLASH_SECONDS * 2; i++ )
        {
            #ifdef WIN32
            Sleep( 500 );
            #else
            usleep( 500000 );
            #endif
            fprintf( stderr, "*" );
        }
        fprintf( stderr, "\n" );

        delete Device.USBDevice;
        Device.USBDevice = new CCyFX3Device();
        res = scan( boot, stream );
        if ( res != FX3_ERR_OK )
            return res;

    }

    res = prepareEndPoints();
    if ( res != FX3_ERR_OK )
        return res;

    bool In;
    int Attr, MaxPktSize, MaxBurst, Interface, Address;
    int EndPointsCount = Endpoints.size();
    for(int i = 0; i < EndPointsCount; i++)
    {
        getEndPointParamsByInd(i, &Attr, &In, &MaxPktSize, &MaxBurst, &Interface, &Address);
        fprintf( stderr, "cy3device::init() EndPoint[%d], Attr=%d, In=%d, MaxPktSize=%d, MaxBurst=%d, Infce=%d, Addr=%d\n",
                 i, Attr, In, MaxPktSize, MaxBurst, Interface, Address);
    }

    return res;
}

void cy3device::CloseDevice()
{
    if (Device.RunStream)
    {
        stopStream();
    }

    if (NULL != Device.USBDevice)
        delete Device.USBDevice;
}

void cy3device::WriteSPI(unsigned char Address, unsigned char Data)
{
    UCHAR buf[16];
    buf[0] = (UCHAR)(Data);
    buf[1] = (UCHAR)(Address);

    CCyControlEndPoint* CtrlEndPt;
    CtrlEndPt = Device.USBDevice->ControlEndPt;
    CtrlEndPt->Target = TGT_DEVICE;
    CtrlEndPt->ReqType = REQ_VENDOR;
    CtrlEndPt->Direction = DIR_TO_DEVICE;
    CtrlEndPt->ReqCode = 0xB3;
    CtrlEndPt->Value = 0;
    CtrlEndPt->Index = 1;
    long len = 16;
    CtrlEndPt->XferData(&buf[0], len);
}

unsigned char cy3device::ReadSPI(unsigned char Address)
{
    UCHAR buf[16];
    //addr |= 0x8000;
    buf[0] = (UCHAR)0xFF;
    buf[1] = (UCHAR)(Address|0x80);

    CCyControlEndPoint* CtrlEndPt;
    CtrlEndPt = Device.USBDevice->ControlEndPt;
    CtrlEndPt->Target = TGT_DEVICE;
    CtrlEndPt->ReqType = REQ_VENDOR;
    CtrlEndPt->Direction = DIR_FROM_DEVICE;
    CtrlEndPt->ReqCode = 0xB5;
    CtrlEndPt->Value = 0;
    CtrlEndPt->Index = Address|0x80;
    long len = 16;
    CtrlEndPt->XferData(&buf[0], len);

    return buf[0];
}

cy3device_err_t cy3device::scan(int &loadable_count, int &streamable_count)
{
    streamable_count = 0;
    loadable_count = 0;
    if (Device.USBDevice == NULL)
    {
        fprintf( stderr, "cy3device::scan() USBDevice == NULL" );
        return FX3_ERR_USB_INIT_FAIL;
    }

    unsigned short product = Device.USBDevice->ProductID;
    unsigned short vendor  = Device.USBDevice->VendorID;
    fprintf( stderr, "Device: 0x%04X 0x%04X ", vendor, product );

    if ( vendor == VENDOR_ID && product == PRODUCT_STREAM )
    {
        fprintf( stderr, " STREAM\n" );
        streamable_count++;
    } else if ( vendor == VENDOR_ID && product == PRODUCT_BOOT )
    {
        fprintf( stderr,  "BOOT\n" );
        loadable_count++;
    }
    fprintf( stderr, "\n" );

    if (loadable_count + streamable_count == 0)
        return FX3_ERR_BAD_DEVICE;
    else
        return FX3_ERR_OK;

}

cy3device_err_t cy3device::prepareEndPoints()
{
    if ( ( Device.USBDevice->VendorID != VENDOR_ID) ||
         ( Device.USBDevice->ProductID != PRODUCT_STREAM ) )
    {
        return FX3_ERR_BAD_DEVICE;
    }

    int interfaces = Device.USBDevice->AltIntfcCount()+1;

    Device.bHighSpeedDevice = Device.USBDevice->bHighSpeed;
    Device.bSuperSpeedDevice = Device.USBDevice->bSuperSpeed;

    for (int i=0; i< interfaces; i++)
    {
        Device.USBDevice->SetAltIntfc(i);

        int eptCnt = Device.USBDevice->EndPointCount();

        // Fill the EndPointsBox
        for (int e=1; e<eptCnt; e++)
        {
            CCyUSBEndPoint *ept = Device.USBDevice->EndPoints[e];
            // INTR, BULK and ISO endpoints are supported.
            if ((ept->Attributes >= 1) && (ept->Attributes <= 3))
            {
                EndPointParams EP;
                EP.Attr = ept->Attributes;
                EP.In = ept->bIn;
                EP.MaxPktSize = ept->MaxPktSize;
                EP.MaxBurst = Device.USBDevice->BcdUSB == 0x0300 ? ept->ssmaxburst : 0;
                EP.Interface = i;
                EP.Address = ept->Address;

                Endpoints.push_back(EP);
            }
        }
    }

    return FX3_ERR_OK;
}

void cy3device::getEndPointParamsByInd(unsigned int EndPointInd, int *Attr, bool *In, int *MaxPktSize, int *MaxBurst, int *Interface, int *Address)
{
    if(EndPointInd >= Endpoints.size())
        return;

    *Attr = Endpoints[EndPointInd].Attr;
    *In = Endpoints[EndPointInd].In;
    *MaxPktSize = Endpoints[EndPointInd].MaxPktSize;
    *MaxBurst = Endpoints[EndPointInd].MaxBurst;
    *Interface = Endpoints[EndPointInd].Interface;
    *Address = Endpoints[EndPointInd].Address;
}

void cy3device::startStream(unsigned int EndPointInd, int PPX, int QueueSize, int TimeOut)
{
    if(EndPointInd >= Endpoints.size())
        return;
    Device.CurEndPoint = Endpoints[EndPointInd];
    Device.PPX = PPX;
    Device.QueueSize = QueueSize;
    Device.TimeOut = TimeOut;

    int alt = Device.CurEndPoint.Interface;
    int eptAddr = Device.CurEndPoint.Address;
    int clrAlt = (Device.USBDevice->AltIntfc() == 0) ? 1 : 0;
    if (! Device.USBDevice->SetAltIntfc(alt))
    {
        Device.USBDevice->SetAltIntfc(clrAlt); // Cleans-up
        return;
    }

    Device.EndPt = Device.USBDevice->EndPointOf((UCHAR)eptAddr);

    if(Device.EndPt->MaxPktSize==0)
        return;

    // Limit total transfer length to 4MByte
    long len = ((Device.EndPt->MaxPktSize) * Device.PPX);

    int maxLen = MAX_TRANSFER_LENGTH;  //4MByte
    if (len > maxLen){
        Device.PPX = maxLen / (Device.EndPt->MaxPktSize);
        if((Device.PPX%8)!=0)
            Device.PPX -= (Device.PPX%8);
    }

    if ((Device.bSuperSpeedDevice || Device.bHighSpeedDevice) && (Device.EndPt->Attributes == 1)){  // HS/SS ISOC Xfers must use PPX >= 8
        Device.PPX = max(Device.PPX, 8);
        Device.PPX = (Device.PPX / 8) * 8;
        if(Device.bHighSpeedDevice)
            Device.PPX = max(Device.PPX, 128);
    }

    BytesXferred = 0;
    Successes = 0;
    Failures = 0;

    // Allocate the arrays needed for queueing
    buffers		= new PUCHAR[Device.QueueSize];
    isoPktInfos	= new CCyIsoPktInfo*[Device.QueueSize];
    contexts		= new PUCHAR[Device.QueueSize];

    Device.TransferSize = ((Device.EndPt->MaxPktSize) * Device.PPX);

    Device.EndPt->SetXferSize(Device.TransferSize);

    // Allocate all the buffers for the queues
    for (int i=0; i< Device.QueueSize; i++)
    {
        buffers[i]        = new UCHAR[Device.TransferSize];
        isoPktInfos[i]    = new CCyIsoPktInfo[Device.PPX];
        inOvLap[i].hEvent = CreateEvent(NULL, false, false, NULL);

        memset(buffers[i],0xEF,Device.TransferSize);
    }

    // Queue-up the first batch of transfer requests
    for (int i=0; i< Device.QueueSize; i++)
    {
        contexts[i] = Device.EndPt->BeginDataXfer(buffers[i], Device.TransferSize, &inOvLap[i]);
        if (Device.EndPt->NtStatus || Device.EndPt->UsbdStatus) // BeginDataXfer failed
        {
            abortTransfer(i+1, buffers,isoPktInfos,contexts,inOvLap);
            return;
        }
    }

    CurrQueue = 0;

    Device.RunStream = true;
    Device.isStreaming = true;
    QMetaObject::invokeMethod( this, "transfer", Qt::QueuedConnection );
}

void cy3device::transfer()
{
    if (!Device.RunStream)
    {
        // Memory clean-up
        abortTransfer(Device.QueueSize, buffers, isoPktInfos, contexts, inOvLap);
        return;
    }

    else
    {
        long rLen = Device.TransferSize;	// Reset this each time through because
        // FinishDataXfer may modify it

        if (!Device.EndPt->WaitForXfer(&inOvLap[CurrQueue], Device.TimeOut))
        {
            Device.EndPt->Abort();
            if (Device.EndPt->LastError == ERROR_IO_PENDING)
                WaitForSingleObject(inOvLap[CurrQueue].hEvent,2000);
        }

        if (Device.EndPt->Attributes == 1) // ISOC Endpoint
        {
            if (Device.EndPt->FinishDataXfer(buffers[CurrQueue], rLen, &inOvLap[CurrQueue], contexts[CurrQueue], isoPktInfos[CurrQueue]))
            {
                CCyIsoPktInfo *pkts = isoPktInfos[CurrQueue];
                for (int j=0; j< Device.PPX; j++)
                {
                    if ((pkts[j].Status == 0) && (pkts[j].Length <= Device.EndPt->MaxPktSize))
                    {
                        BytesXferred += pkts[j].Length;
                        Successes++;
                    }
                    else
                        Failures++;
                    pkts[j].Length = 0;	// Reset to zero for re-use.
                    pkts[j].Status = 0;
                }
            }
            else
                Failures++;
        }
        else // BULK Endpoint
        {
            if (Device.EndPt->FinishDataXfer(buffers[CurrQueue], rLen, &inOvLap[CurrQueue], contexts[CurrQueue]))
            {
                Successes++;
                BytesXferred += rLen;
            }
            else
                Failures++;
        }

        BytesXferred = max(BytesXferred, 0);

        processData((char*)buffers[CurrQueue], rLen);

        // Re-submit this queue element to keep the queue full
        contexts[CurrQueue] = Device.EndPt->BeginDataXfer(buffers[CurrQueue], Device.TransferSize, &inOvLap[CurrQueue]);
        if (Device.EndPt->NtStatus || Device.EndPt->UsbdStatus) // BeginDataXfer failed
        {
            abortTransfer(Device.QueueSize, buffers, isoPktInfos, contexts, inOvLap);
            return;
        }

        CurrQueue = (CurrQueue + 1) % Device.QueueSize;

        QMetaObject::invokeMethod( this, "transfer", Qt::QueuedConnection );
    }  // End of the transfer loop
}

void cy3device::abortTransfer(int pending, PUCHAR *buffers, CCyIsoPktInfo **isoPktInfos, PUCHAR *contexts, OVERLAPPED *inOvLap)
{
    long len = Device.EndPt->MaxPktSize * Device.PPX;

    for (int j=0; j< Device.QueueSize; j++)
    {
        if (j<pending)
        {
            if (!Device.EndPt->WaitForXfer(&inOvLap[j], Device.TimeOut))
            {
                Device.EndPt->Abort();
                if (Device.EndPt->LastError == ERROR_IO_PENDING)
                    WaitForSingleObject(inOvLap[j].hEvent,2000);
            }

            Device.EndPt->FinishDataXfer(buffers[j], len, &inOvLap[j], contexts[j]);
        }

        CloseHandle(inOvLap[j].hEvent);

        delete [] buffers[j];
        delete [] isoPktInfos[j];
    }

    delete [] buffers;
    delete [] isoPktInfos;
    delete [] contexts;

    Device.RunStream = false;
    Device.isStreaming = false;
}

void cy3device::stopStream()
{
    if(!Device.RunStream)
        return;
    Device.RunStream = false;

    while(Device.isStreaming)
        Sleep(1);
}

unsigned int count_size = 0;
unsigned short mask = 0;

int testDataBits16(unsigned short* data, int size)
{
    for ( int scount = 0; scount < size/2; scount++ )
        mask |= data[scount];
    if (count_size >= 64*1024*1024)
    {
        qDebug() << "mask: " << hex << mask;
        count_size = 0;
        //mask = 0;
    }
    else
        count_size += size;

    return mask;
}

void cy3device::processData(char *data, int size)
{
    testDataBits16((unsigned short*) data, size);
}
