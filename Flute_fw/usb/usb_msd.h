/*
 * usb_mst.h
 *
 *  Created on: 2016/01/29 �.
 *      Author: Kreyl
 */

#ifndef USB_MSD_H__
#define USB_MSD_H__

#include "hal.h"
#include "scsi.h"

/*
 * Beware! 24MHz core clock is not enough for USB to be happy!
 */

#if 1 // ================= Mass Storage constants and types ====================
// Enum for the Mass Storage class specific control requests that can be issued by the USB bus host
enum MS_ClassRequests_t {
    // Mass Storage class-specific request to retrieve the total number of Logical Units (drives) in the SCSI device.
    MS_REQ_GetMaxLUN = 0xFE,
    // Mass Storage class-specific request to reset the Mass Storage interface, ready for the next command.
    MS_REQ_MassStorageReset = 0xFF,
};

// Mass Storage Class Command Block Wrapper
struct MS_CommandBlockWrapper_t {
    uint32_t Signature;         // Command block signature, must be MS_CBW_SIGNATURE to indicate a valid Command Block
    uint32_t Tag;               // Unique command ID value, to associate a command block wrapper with its command status wrapper
    uint32_t DataTransferLen;   // Length of the optional data portion of the issued command, in bytes
    uint8_t  Flags;             // Command block flags, indicating command data direction
    uint8_t  LUN;               // Logical Unit number this command is issued to
    uint8_t  SCSICmdLen;        // Length of the issued SCSI command within the SCSI command data array
    uint8_t  SCSICmdData[16];   // Issued SCSI command in the Command Block
} __attribute__ ((__packed__));
#define MS_CMD_SZ   sizeof(MS_CommandBlockWrapper_t)

// Mass Storage Class Command Status Wrapper
struct MS_CommandStatusWrapper_t {
    uint32_t Signature;          // Status block signature, must be \ref MS_CSW_SIGNATURE to indicate a valid Command Status
    uint32_t Tag;                // Unique command ID value, to associate a command block wrapper with its command status wrapper
    uint32_t DataTransferResidue;// Number of bytes of data not processed in the SCSI command
    uint8_t  Status;             // Status code of the issued command - a value from the MS_CommandStatusCodes_t enum
} __attribute__ ((__packed__));
#endif

#define MSD_TIMEOUT_MS   2700
#define MSD_DATABUF_SZ   4096

class UsbMsd_t {
private:
    MS_CommandBlockWrapper_t CmdBlock;
    MS_CommandStatusWrapper_t CmdStatus;
    SCSI_RequestSenseResponse_t SenseData;
    SCSI_ReadCapacity10Response_t ReadCapacity10Response;
    SCSI_ReadFormatCapacitiesResponse_t ReadFormatCapacitiesResponse;
    void SCSICmdHandler();
    // Scsi commands
    void CmdTestReady();
    uint8_t CmdStartStopUnit();
    uint8_t CmdInquiry();
    uint8_t CmdRequestSense();
    uint8_t CmdReadCapacity10();
    uint8_t CmdSendDiagnostic();
    uint8_t CmdReadFormatCapacities();
    uint8_t CmdRead10();
    uint8_t CmdWrite10();
    uint8_t CmdModeSense6();
    uint8_t ReadWriteCommon(uint32_t *PAddr, uint16_t *PLen);
    struct {
        uint8_t Buf[MSD_DATABUF_SZ];
    } __attribute__((aligned(MSD_DATABUF_SZ / 4)));
    void BusyWaitIN();
    uint8_t BusyWaitOUT();
    void TransmitBuf(uint8_t *Ptr, uint32_t Len);
    uint8_t ReceiveToBuf(uint8_t *Ptr, uint32_t Len);
public:
    void Init();
    void Reset();
    void Connect();
    void Disconnect();
    // Inner use
    void Task();
    thread_t *PThread;
    bool ISayIsReady = true;
};

extern UsbMsd_t UsbMsd;

#endif //USB_MSD_H__