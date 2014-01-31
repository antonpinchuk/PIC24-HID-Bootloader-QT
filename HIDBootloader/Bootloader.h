#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include "DeviceData.h"
#include "Device.h"
#include "ImportExportHex.h"
#include "Comm.h"

/*!
 * Provides HID bootloader communication.
 */
class Bootloader : public QObject {
    Q_OBJECT


public slots:
   void writeLog(Qstring value);
signals:
   void readLog(Qstring value);

protected:

    bool deviceFirmwareIsAtLeast101 = false;
    Comm::ExtendedQueryInfo extendedBootInfo;




    QString fileName, watchFileName;


public:    

    Comm* comm;
    DeviceData* deviceData;
    DeviceData* hexData;
    Device* device;


    bool writeFlash;
    bool writeEeprom;
    bool writeConfig;
    bool eraseDuringWrite;
    bool hexOpen;

    bool wasBootloaderMode;

    void GetQuery(void);
    void EraseDevice(void);
    void BlankCheckDevice(void);
    void WriteDevice(void);
    void VerifyDevice(void);

    void LoadFile(QString fileName);

    explicit Bootloader();
    ~Bootloader();

};

#endif // BOOTLOADER_H

