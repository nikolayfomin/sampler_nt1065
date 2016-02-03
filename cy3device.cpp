#include "cy3device.h"

const char* cy3device_get_error_string(cy3device_err_t error) {
    switch ( error ) {
        case CY3DEV_OK:                        return "FX3_ERR_OK";
        case CY3DEV_ERR_DRV_NOT_IMPLEMENTED:       return "FX3_ERR_DRV_NOT_IMPLEMENTED";
        case CY3DEV_ERR_USB_INIT_FAIL:             return "FX3_ERR_USB_INIT_FAIL";
        case CY3DEV_ERR_NO_DEVICE_FOUND:           return "FX3_ERR_NO_DEVICE_FOUND";
        case CY3DEV_ERR_BAD_DEVICE:                return "FX3_ERR_BAD_DEVICE";
        case CY3DEV_ERR_FIRMWARE_FILE_IO_ERROR:    return "FX3_ERR_FIRMWARE_FILE_IO_ERROR";
        case CY3DEV_ERR_FIRMWARE_FILE_CORRUPTED:   return "FX3_ERR_FIRMWARE_FILE_CORRUPTED";
        case CY3DEV_ERR_ADDFIRMWARE_FILE_IO_ERROR: return "FX3_ERR_ADDFIRMWARE_FILE_IO_ERROR";
        case CY3DEV_ERR_REG_WRITE_FAIL:            return "FX3_ERR_REG_WRITE_FAIL";
        case CY3DEV_ERR_FW_TOO_MANY_ERRORS:        return "FX3_ERR_FW_TOO_MANY_ERRORS";
        case CY3DEV_ERR_CTRL_TX_FAIL:              return "FX3_ERR_CTRL_TX_FAIL";
        default:                                return "FX3_ERR_UNKNOWN_ERROR";
    }

}

cy3device::cy3device(const char* firmwareFileName, QObject *parent) : QObject(parent)
{
    FWName = firmwareFileName;

    BytesXferred = 0;
    Successes = 0;
    Failures = 0;

    CurrQueue = 0;

    isStreaming = false;
    Params.RunStream = false;
}

cy3device_err_t cy3device::OpenDevice()
{
    if (Params.USBDevice == NULL)
        Params.USBDevice = new CCyFX3Device();

    isStreaming = false;
    Params.RunStream = false;

    int boot = 0;
    int stream = 0;
    cy3device_err_t res = scan( boot, stream );
    if ( res != CY3DEV_OK )
        return res;

    bool need_fw_load = stream == 0 && boot > 0;

    if ( need_fw_load )
    {
        qDebug("cy3device::OpenDevice() fw load needed\n");

        QFileInfo checkFile(FWName);

        if (!(checkFile.exists() && checkFile.isFile()))
            return CY3DEV_ERR_FIRMWARE_FILE_IO_ERROR;

        if ( Params.USBDevice->IsBootLoaderRunning() )
        {
            int retCode  = Params.USBDevice->DownloadFw((char*)FWName.toStdString().c_str(), FX3_FWDWNLOAD_MEDIA_TYPE::RAM);

            if ( retCode != FX3_FWDWNLOAD_ERROR_CODE::SUCCESS )
            {
                switch( retCode )
                {
                    case INVALID_FILE:
                    case CORRUPT_FIRMWARE_IMAGE_FILE:
                    case INCORRECT_IMAGE_LENGTH:
                    case INVALID_FWSIGNATURE:
                        return CY3DEV_ERR_FIRMWARE_FILE_CORRUPTED;
                    default:
                        return CY3DEV_ERR_CTRL_TX_FAIL;
                }
            } else
                qDebug("cy3device::OpenDevice() boot ok!\n" );

        } else
        {
            qDebug("__error__ cy3device::OpenDevice() StartParams.USBDevice->IsBootLoaderRunning() is FALSE\n" );
            return CY3DEV_ERR_BAD_DEVICE;
        }
    }

    if ( need_fw_load )
    {
        int PAUSE_AFTER_FLASH_SECONDS = 2;
        qDebug("cy3device::OpenDevice() flash completed!\nPlease wait for %d seconds\n", PAUSE_AFTER_FLASH_SECONDS );
        for ( int i = 0; i < PAUSE_AFTER_FLASH_SECONDS * 2; i++ )
        {
            #ifdef WIN32
            Sleep( 500 );
            #else
            usleep( 500000 );
            #endif
            qDebug("*" );
        }
        qDebug("\n" );

        delete Params.USBDevice;
        Params.USBDevice = new CCyFX3Device();
        res = scan( boot, stream );
        if ( res != CY3DEV_OK )
            return res;

    }

    res = prepareEndPoints();
    if ( res != CY3DEV_OK )
        return res;

    emit DebugMessage(Params.bSuperSpeedDevice ? QString("SuperSpeed USB") :
                      Params.bHighSpeedDevice ? QString("HighSpeed USB") :
                                                QString("FullSpeed USB"));

    bool In;
    int Attr, MaxPktSize, MaxBurst, Interface, Address;
    int EndPointsCount = Endpoints.size();
    for(int i = 0; i < EndPointsCount; i++)
    {
        getEndPointParamsByInd(i, &Attr, &In, &MaxPktSize, &MaxBurst, &Interface, &Address);
        qDebug("cy3device::OpenDevice() EndPoint[%d], Attr=%d, In=%d, MaxPktSize=%d, MaxBurst=%d, Infce=%d, Addr=%d\n",
                 i, Attr, In, MaxPktSize, MaxBurst, Interface, Address);
        emit DebugMessage(QString("Endpoint %1, MaxPacketSize %2, MaxBurst %3").arg(i).arg(MaxPktSize).arg(MaxBurst));
    }

    return res;
}

void cy3device::CloseDevice()
{
    if (NULL != Params.USBDevice)
    {
        Params.USBDevice->Close();
        delete Params.USBDevice;
        Params.USBDevice = NULL;
    }
    qDebug("cy3device::CloseDevice completed");
}

cy3device_err_t cy3device::WriteSPI(unsigned char Address, unsigned char Data)
{
    if (Params.USBDevice == NULL || !Params.USBDevice->IsOpen())
    {
        qDebug("cy3device::WriteSPI device error");
        return CY3DEV_ERR_BAD_DEVICE;
    }

    UCHAR buf[16];
    buf[0] = (UCHAR)(Data);
    buf[1] = (UCHAR)(Address);

    CCyControlEndPoint* CtrlEndPt;
    CtrlEndPt = Params.USBDevice->ControlEndPt;
    CtrlEndPt->Target = TGT_DEVICE;
    CtrlEndPt->ReqType = REQ_VENDOR;
    CtrlEndPt->Direction = DIR_TO_DEVICE;
    CtrlEndPt->ReqCode = 0xB3;
    CtrlEndPt->Value = 0;
    CtrlEndPt->Index = 1;
    long len = 16;
    if (CtrlEndPt->XferData(&buf[0], len))
    {
        qDebug("cy3device::WriteSPI ok, Address %hhu, Data 0x%02X", Address, Data);
        return CY3DEV_OK;
    }
    else
    {
        qDebug("cy3device::WriteSPI fail");
        return CY3DEV_ERR_CTRL_TX_FAIL;
    }
}

cy3device_err_t cy3device::ReadSPI(unsigned char Address, unsigned char* Data)
{
    if (Params.USBDevice == NULL || !Params.USBDevice->IsOpen())
    {
        qDebug("cy3device::ReadSPI device error");
        return CY3DEV_ERR_BAD_DEVICE;
    }

    UCHAR buf[16];
    //addr |= 0x8000;
    buf[0] = (UCHAR)0xFF;
    buf[1] = (UCHAR)(Address|0x80);

    CCyControlEndPoint* CtrlEndPt;
    CtrlEndPt = Params.USBDevice->ControlEndPt;
    CtrlEndPt->Target = TGT_DEVICE;
    CtrlEndPt->ReqType = REQ_VENDOR;
    CtrlEndPt->Direction = DIR_FROM_DEVICE;
    CtrlEndPt->ReqCode = 0xB5;
    CtrlEndPt->Value = 0;
    CtrlEndPt->Index = Address|0x80;
    long len = 16;
    if (CtrlEndPt->XferData(&buf[0], len))
    {
        *Data = buf[0];
        qDebug("cy3device::ReadSPI ok, Address %hhu, Data 0x%02X", Address, Data[0]);
        return CY3DEV_OK;
    }
    else
    {
        qDebug("cy3device::ReadSPI fail");
        return CY3DEV_ERR_CTRL_TX_FAIL;
    }
}

void cy3device::StartStream()
{
    startTransfer(0, 128, 4, 1500);
}

void cy3device::StopStream()
{
    stopTransfer();
}

cy3device_err_t cy3device::scan(int &loadable_count, int &streamable_count)
{
    streamable_count = 0;
    loadable_count = 0;
    if (Params.USBDevice == NULL)
    {
        qDebug("cy3device::scan() USBDevice == NULL" );
        return CY3DEV_ERR_USB_INIT_FAIL;
    }

    unsigned short product = Params.USBDevice->ProductID;
    unsigned short vendor  = Params.USBDevice->VendorID;
    qDebug("Device: 0x%04X 0x%04X ", vendor, product );

    if ( vendor == VENDOR_ID && product == PRODUCT_STREAM )
    {
        qDebug("STREAM\n" );
        streamable_count++;
    } else if ( vendor == VENDOR_ID && product == PRODUCT_BOOT )
    {
        qDebug("BOOT\n" );
        loadable_count++;
    }
    qDebug("\n" );

    if (loadable_count + streamable_count == 0)
        return CY3DEV_ERR_BAD_DEVICE;
    else
        return CY3DEV_OK;

}

cy3device_err_t cy3device::prepareEndPoints()
{
    if ( ( Params.USBDevice->VendorID != VENDOR_ID) ||
         ( Params.USBDevice->ProductID != PRODUCT_STREAM ) )
    {
        return CY3DEV_ERR_BAD_DEVICE;
    }

    int interfaces = Params.USBDevice->AltIntfcCount()+1;

    Params.bHighSpeedDevice = Params.USBDevice->bHighSpeed;
    Params.bSuperSpeedDevice = Params.USBDevice->bSuperSpeed;

    Endpoints.clear();

    for (int i=0; i< interfaces; i++)
    {
        Params.USBDevice->SetAltIntfc(i);

        int eptCnt = Params.USBDevice->EndPointCount();

        // Fill the EndPointsBox
        for (int e=1; e<eptCnt; e++)
        {
            CCyUSBEndPoint *ept = Params.USBDevice->EndPoints[e];
            // INTR, BULK and ISO endpoints are supported.
            if ((ept->Attributes >= 1) && (ept->Attributes <= 3))
            {
                EndPointParams EP;
                EP.Attr = ept->Attributes;
                EP.In = ept->bIn;
                EP.MaxPktSize = ept->MaxPktSize;
                EP.MaxBurst = Params.USBDevice->BcdUSB == 0x0300 ? ept->ssmaxburst : 0;
                EP.Interface = i;
                EP.Address = ept->Address;

                Endpoints.push_back(EP);
            }
        }
    }

    return CY3DEV_OK;
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

cy3device_err_t cy3device::startTransfer(unsigned int EndPointInd, int PPX, int QueueSize, int TimeOut)
{
    if (Params.USBDevice == NULL || !Params.USBDevice->IsOpen())
    {
        qDebug("cy3device::startTransfer device error");
        return CY3DEV_ERR_BAD_DEVICE;
    }

    if(EndPointInd >= Endpoints.size())
    {
        qDebug("cy3device::startTransfer no endpoints");
        return CY3DEV_ERR_BAD_DEVICE;
    }

    Params.CurEndPoint = Endpoints[EndPointInd];
    Params.PPX = PPX;
    Params.QueueSize = QueueSize;
    Params.TimeOut = TimeOut;

    int alt = Params.CurEndPoint.Interface;
    int eptAddr = Params.CurEndPoint.Address;
    int clrAlt = (Params.USBDevice->AltIntfc() == 0) ? 1 : 0;
    if (! Params.USBDevice->SetAltIntfc(alt))
    {
        Params.USBDevice->SetAltIntfc(clrAlt); // Cleans-up
        qDebug("cy3device::startTransfer interface error");
        return CY3DEV_ERR_BAD_DEVICE;
    }

    Params.EndPt = Params.USBDevice->EndPointOf((UCHAR)eptAddr);

    if(Params.EndPt->MaxPktSize==0)
    {
        qDebug("cy3device::startTransfer endpoint transfer size zero");
        return CY3DEV_ERR_BAD_DEVICE;
    }

    // Limit total transfer length to 4MByte
    long len = ((Params.EndPt->MaxPktSize) * Params.PPX);

    int maxLen = MAX_TRANSFER_LENGTH;  //4MByte
    if (len > maxLen){
        Params.PPX = maxLen / (Params.EndPt->MaxPktSize);
        if((Params.PPX%8)!=0)
            Params.PPX -= (Params.PPX%8);
    }

    if ((Params.bSuperSpeedDevice || Params.bHighSpeedDevice) && (Params.EndPt->Attributes == 1)){  // HS/SS ISOC Xfers must use PPX >= 8
        Params.PPX = max(Params.PPX, 8);
        Params.PPX = (Params.PPX / 8) * 8;
        if(Params.bHighSpeedDevice)
            Params.PPX = max(Params.PPX, 128);
    }

    BytesXferred = 0;
    Successes = 0;
    Failures = 0;

    // Allocate the arrays needed for queueing
    buffers		= new PUCHAR[Params.QueueSize];
    isoPktInfos	= new CCyIsoPktInfo*[Params.QueueSize];
    contexts		= new PUCHAR[Params.QueueSize];

    Params.TransferSize = ((Params.EndPt->MaxPktSize) * Params.PPX);

    Params.EndPt->SetXferSize(Params.TransferSize);

    // Allocate all the buffers for the queues
    for (int i=0; i< Params.QueueSize; i++)
    {
        buffers[i]        = new UCHAR[Params.TransferSize];
        isoPktInfos[i]    = new CCyIsoPktInfo[Params.PPX];
        inOvLap[i].hEvent = CreateEvent(NULL, false, false, NULL);

        memset(buffers[i],0xEF,Params.TransferSize);
    }

    // Queue-up the first batch of transfer requests
    for (int i=0; i< Params.QueueSize; i++)
    {
        contexts[i] = Params.EndPt->BeginDataXfer(buffers[i], Params.TransferSize, &inOvLap[i]);
        if (Params.EndPt->NtStatus || Params.EndPt->UsbdStatus) // BeginDataXfer failed
        {
            abortTransfer(i+1, buffers,isoPktInfos,contexts,inOvLap);
            qDebug("cy3device::startTransfer BeginDataXfer failed");
            return CY3DEV_ERR_BULK_IO_ERROR;
        }
    }

    CurrQueue = 0;

    qDebug("cy3device::startTransfer initiated");

    time.start();

    Params.RunStream = true;
    isStreaming = true;
    QMetaObject::invokeMethod( this, "transfer", Qt::QueuedConnection );

    return CY3DEV_OK;
}

void cy3device::transfer()
{
    if (!Params.RunStream)
    {
        // Memory clean-up
        abortTransfer(Params.QueueSize, buffers, isoPktInfos, contexts, inOvLap);
        return;
    }

    else
    {
        long rLen = Params.TransferSize;	// Reset this each time through because
        // FinishDataXfer may modify it

        if (!Params.EndPt->WaitForXfer(&inOvLap[CurrQueue], Params.TimeOut))
        {
            Params.EndPt->Abort();
            if (Params.EndPt->LastError == ERROR_IO_PENDING)
                WaitForSingleObject(inOvLap[CurrQueue].hEvent,2000);
        }

        if (Params.EndPt->Attributes == 1) // ISOC Endpoint
        {
            if (Params.EndPt->FinishDataXfer(buffers[CurrQueue], rLen, &inOvLap[CurrQueue], contexts[CurrQueue], isoPktInfos[CurrQueue]))
            {
                CCyIsoPktInfo *pkts = isoPktInfos[CurrQueue];
                for (int j=0; j< Params.PPX; j++)
                {
                    if ((pkts[j].Status == 0) && (pkts[j].Length <= Params.EndPt->MaxPktSize))
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
            if (Params.EndPt->FinishDataXfer(buffers[CurrQueue], rLen, &inOvLap[CurrQueue], contexts[CurrQueue]))
            {
                Successes++;
                BytesXferred += rLen;
            }
            else
                Failures++;
        }

        if (time.elapsed() > 1000)
        {
            emit ReportBandwidth(BytesXferred/time.elapsed()*1000);
            BytesXferred = 0;
            time.restart();
        }

        processData((char*)buffers[CurrQueue], rLen);

        // Re-submit this queue element to keep the queue full
        contexts[CurrQueue] = Params.EndPt->BeginDataXfer(buffers[CurrQueue], Params.TransferSize, &inOvLap[CurrQueue]);
        if (Params.EndPt->NtStatus || Params.EndPt->UsbdStatus) // BeginDataXfer failed
        {
            abortTransfer(Params.QueueSize, buffers, isoPktInfos, contexts, inOvLap);
            qDebug("cy3device::transfer() BeginDataXfer failed");
            return;
        }

        CurrQueue = (CurrQueue + 1) % Params.QueueSize;

        QMetaObject::invokeMethod( this, "transfer", Qt::QueuedConnection );
    }  // End of the transfer loop
}

void cy3device::abortTransfer(int pending, PUCHAR *buffers, CCyIsoPktInfo **isoPktInfos, PUCHAR *contexts, OVERLAPPED *inOvLap)
{
    long len = Params.EndPt->MaxPktSize * Params.PPX;

    for (int j=0; j< Params.QueueSize; j++)
    {
        if (j<pending)
        {
            if (!Params.EndPt->WaitForXfer(&inOvLap[j], Params.TimeOut))
            {
                Params.EndPt->Abort();
                if (Params.EndPt->LastError == ERROR_IO_PENDING)
                    WaitForSingleObject(inOvLap[j].hEvent,2000);
            }

            Params.EndPt->FinishDataXfer(buffers[j], len, &inOvLap[j], contexts[j]);
        }

        CloseHandle(inOvLap[j].hEvent);

        delete [] buffers[j];
        delete [] isoPktInfos[j];
    }

    delete [] buffers;
    delete [] isoPktInfos;
    delete [] contexts;

    Params.RunStream = false;
    isStreaming = false;
    qDebug("cy3device::abortTransfer completed");
}

void cy3device::stopTransfer()
{
    if(!Params.RunStream)
        return;
    Params.RunStream = false;
}

unsigned int count_size = 0;
unsigned short mask = 0;

int testDataBits16(unsigned short* data, int size)
{
    for ( int scount = 0; scount < size; scount++ )
        mask |= data[scount];
    if (count_size >= 64*1024*1024)
    {
        qDebug() << "mask: " << hex << mask;
        count_size = 0;
        mask = 0;
    }
    else
        count_size += size;

    return mask;
}

void cy3device::processData(char *data, int size)
{
    unsigned short* sdata = (unsigned short*) data;
    int ssize = size/2;
    //testDataBits16(sdata, ssize);

    QVector<unsigned short> qdata;
    qdata.resize(ssize);
    std::copy(sdata, sdata+ssize, qdata.begin());

    emit RawData(qdata);
}
