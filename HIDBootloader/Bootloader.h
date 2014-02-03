#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include <QTime>
#include <QTextStream>
#include <QFileInfo>
#include <QString>
#include <QByteArray>


#include "DeviceData.h"
#include "Device.h"
#include "ImportExportHex.h"
#include "Comm.h"

/**
 * Bootloader
 *
 * Implements Microchip bootloader protocol, sends signals about connection and progress state
 */
class Bootloader : public QObject {
    Q_OBJECT

public:
    enum MessageType {
        Info = 0, Warning, Error, Other = 0xFF
    };

signals:
    void setConnected(bool enable);
    void setBootloadEnabled(bool enable);
    void setBootloadBusy(bool busy);
    void setProgressBar(int newValue);
    void message(Bootloader::MessageType type, QString value);
    void messageClear();
    void ReadComplete();
    void WriteComplete();


public:    
    Comm* comm;
    Device* device;
    DeviceData* deviceData;

    bool hexOpen;

    bool writeFlash;
    bool writeEeprom;
    bool writeConfig;

    bool wasBootloaderMode;
    bool eraseDuringWrite;
    unsigned char rangeReadWrite;

    void GetQuery(void);
    void EraseDevice(void);
    void BlankCheckDevice(void);
    void WriteDevice(void);
    void VerifyDevice(void);
    void ReadDevice(void);

    HexImporter::ErrorCode LoadFile(QString fileName);
    void SaveFile(QString fileName);

    QString GetMemoryRangeNameByType(int value);

    explicit Bootloader();
    ~Bootloader();

private slots:
    void Connection(void);
    void commProgressBar(int newValue);

protected:
    QTimer *timer;

    Comm::ExtendedQueryInfo extendedBootInfo;

    void IoWithDeviceStarted(QString msg);
    void IoWithDeviceCompleted(QString msg, Comm::ErrorCode result, double time);

    Comm::ErrorCode RemapInterruptVectors(Device* device, DeviceData* deviceData);

private:
    DeviceData* hexData;

    bool deviceFirmwareIsAtLeast101;

    bool pollUsbEnabled;
   
    void log(QString value);
    void log(MessageType type, QString value);
    void logClear();

};

#endif // BOOTLOADER_H

