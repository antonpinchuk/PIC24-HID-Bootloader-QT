#include "Bootloader.h"

#include <QTime>
#include <QSettings>
#include <QTextStream>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>

/**
 *
 */
Bootloader::Bootloader() {    
    comm = new Comm();
    deviceData = new DeviceData();
    hexData = new DeviceData();

    device = new Device(deviceData);
}

/**
 *
 */
Bootloader::~Bootloader() {

    comm->close();
    delete comm;
    delete deviceData;
    delete hexData;
    delete device;
}


//Surely the micro doesn't have a programmable memory region greater than 268 Megabytes...
//Value used for error checking device reponse values.
#define MAXIMUM_PROGRAMMABLE_MEMORY_SEGMENT_SIZE 0x0FFFFFFF


void Bootloader::IoWithDeviceStart(QString msg)
{
    emit writeLog(msg);
    emit setBootloadBusy(true);
}


void Bootloader::IoWithDeviceComplet(QString msg, Comm::ErrorCode result, double time)
{
    QTextStream ss(&msg);

    switch(result)
    {
        case Comm::Success:
            ss << " Complete (" << time << "s)\n";
            emit setBootloadBusy(false);
            break;
        case Comm::NotConnected:
            ss << " Failed. Device not connected.\n";
            emit setBootloadBusy(false);
            break;
        case Comm::Fail:
            ss << " Failed.\n";
            emit setBootloadBusy(false);
            break;
        case Comm::IncorrectCommand:
            ss << " Failed. Unable to communicate with device.\n";
            emit setBootloadBusy(false);
            break;
        case Comm::Timeout:
            ss << " Timed out waiting for device (" << time << "s)\n";
            emit setBootloadBusy(false);
            break;
        default:
            break;
    }
    emit writeLog(msg);
}

//Routine that verifies the contents of the non-voltaile memory regions in the device, after an erase/programming cycle.
//This function requests the memory contents of the device, then compares it against the parsed .hex file data to make sure
//The locations that got programmed properly match.
void Bootloader::VerifyDevice()
{
    Comm::ErrorCode result;
    DeviceData::MemoryRange deviceRange, hexRange;
    QTime elapsed;
    unsigned int i, j;
    unsigned int arrayIndex;
    bool failureDetected = false;
    unsigned char flashData[MAX_ERASE_BLOCK_SIZE];
    unsigned char hexEraseBlockData[MAX_ERASE_BLOCK_SIZE];
    unsigned long int startOfEraseBlock;

    //Initialize an erase block sized buffer with 0xFF.
    //Used later for post SIGN_FLASH verify operation.
    memset(&hexEraseBlockData[0], 0xFF, MAX_ERASE_BLOCK_SIZE);

    emit writeLog("Verifying Device...");
    foreach(deviceRange, deviceData->ranges)
    {
        if(writeFlash && (deviceRange.type == PROGRAM_MEMORY))
        {
            elapsed.start();
            emit writeLog("Verifying Device's Program Memory...");

            result = comm->GetData(deviceRange.start,
                                   device->bytesPerPacket,
                                   device->bytesPerAddressFLASH,
                                   device->bytesPerWordFLASH,
                                   deviceRange.end,
                                   deviceRange.pDataBuffer);

            if(result != Comm::Success)
            {
                failureDetected = true;
                qWarning("Error reading device.");
                IoWithDeviceComplet("Verifying Device's Program Memory", result, ((double)elapsed.elapsed()) / 1000);
            }

            //Search through all of the programmable memory regions from the parsed .hex file data.
            //For each of the programmable memory regions found, if the region also overlaps a region
            //that was included in the device programmed area (which just got read back with GetData()),
            //then verify both the parsed hex contents and read back data match.
            foreach(hexRange, hexData->ranges)
            {
                if(deviceRange.start == hexRange.start)
                {
                    //For this entire programmable memory address range, check to see if the data read from the device exactly
                    //matches what was in the hex file.
                    for(i = deviceRange.start; i < deviceRange.end; i++)
                    {
                        //For each byte of each device address (1 on PIC18, 2 on PIC24, since flash memory is 16-bit WORD array)
                        for(j = 0; j < device->bytesPerAddressFLASH; j++)
                        {
                            //Check if the device response data matches the data we parsed from the original input .hex file.
                            if(deviceRange.pDataBuffer[((i - deviceRange.start) * device->bytesPerAddressFLASH)+j] != hexRange.pDataBuffer[((i - deviceRange.start) * device->bytesPerAddressFLASH)+j])
                            {
                                //A mismatch was detected.

                                //Check if this is a PIC24 device and we are looking at the "phantom byte"
                                //(upper byte [j = 1] of odd address [i%2 == 1] 16-bit flash words).  If the hex data doesn't match
                                //the device (which should be = 0x00 for these locations), this isn't a real verify
                                //failure, since value is a don't care anyway.  This could occur if the hex file imported
                                //doesn't contain all locations, and we "filled" the region with pure 0xFFFFFFFF, instead of 0x00FFFFFF
                                //when parsing the hex file.
                                if((device->family == Device::PIC24) && ((i % 2) == 1) && (j == 1))
                                {
                                    //Not a real verify failure, phantom byte is unimplemented and is a don't care.
                                }
                                else
                                {
                                    //If the data wasn't a match, and this wasn't a PIC24 phantom byte, then if we get
                                    //here this means we found a true verify failure.
                                    failureDetected = true;
                                    if(device->family == Device::PIC24)
                                    {
                                        qWarning("Device: 0x%x Hex: 0x%x", *(unsigned short int*)&deviceRange.pDataBuffer[((i - deviceRange.start) * device->bytesPerAddressFLASH)+j], *(unsigned short int*)&hexRange.pDataBuffer[((i - deviceRange.start) * device->bytesPerAddressFLASH)+j]);
                                    }
                                    else
                                    {
                                        qWarning("Device: 0x%x Hex: 0x%x", deviceRange.pDataBuffer[((i - deviceRange.start) * device->bytesPerAddressFLASH)+j], hexRange.pDataBuffer[((i - deviceRange.start) * device->bytesPerAddressFLASH)+j]);
                                    }
                                    qWarning("Failed verify at address 0x%x", i);
                                    IoWithDeviceComplet("Verify", Comm::Fail, ((double)elapsed.elapsed()) / 1000);
                                    return;
                                }
                            }//if(deviceRange.pDataBuffer[((i - deviceRange.start) * device->bytesPerAddressFLASH)+j] != hexRange.pDataBuffer[((i - deviceRange.start) * device->bytesPerAddressFLASH)+j])
                        }//for(j = 0; j < device->bytesPerAddressFLASH; j++)
                    }//for(i = deviceRange.start; i < deviceRange.end; i++)
                }//if(deviceRange.start == hexRange.start)
            }//foreach(hexRange, hexData->ranges)
            IoWithDeviceComplet("Verify", Comm::Success, ((double)elapsed.elapsed()) / 1000);
        }//if(writeFlash && (deviceRange.type == PROGRAM_MEMORY))
        else if (writeEeprom && (deviceRange.type == EEPROM_MEMORY)) {
            elapsed.start();
            IoWithDeviceStart("Verifying Device's EEPROM Memory...");

            result = comm->GetData(deviceRange.start,
                                   device->bytesPerPacket,
                                   device->bytesPerAddressEEPROM,
                                   device->bytesPerWordEEPROM,
                                   deviceRange.end,
                                   deviceRange.pDataBuffer);

            if(result != Comm::Success) {
                failureDetected = true;
                qWarning("Error reading device.");
                IoWithDeviceComplet("Verifying Device's EEPROM Memory", result, ((double)elapsed.elapsed()) / 1000);
            }


            //Search through all of the programmable memory regions from the parsed .hex file data.
            //For each of the programmable memory regions found, if the region also overlaps a region
            //that was included in the device programmed area (which just got read back with GetData()),
            //then verify both the parsed hex contents and read back data match.
            foreach(hexRange, hexData->ranges)
            {
                if(deviceRange.start == hexRange.start)
                {
                    //For this entire programmable memory address range, check to see if the data read from the device exactly
                    //matches what was in the hex file.
                    for(i = deviceRange.start; i < deviceRange.end; i++)
                    {
                        //For each byte of each device address (only 1 for EEPROM byte arrays, presumably 2 for EEPROM WORD arrays)
                        for(j = 0; j < device->bytesPerAddressEEPROM; j++)
                        {
                            //Check if the device response data matches the data we parsed from the original input .hex file.
                            if(deviceRange.pDataBuffer[((i - deviceRange.start) * device->bytesPerAddressEEPROM)+j] != hexRange.pDataBuffer[((i - deviceRange.start) * device->bytesPerAddressEEPROM)+j])
                            {
                                //A mismatch was detected.
                                failureDetected = true;
                                qWarning("Device: 0x%x Hex: 0x%x", deviceRange.pDataBuffer[((i - deviceRange.start) * device->bytesPerAddressFLASH)+j], hexRange.pDataBuffer[((i - deviceRange.start) * device->bytesPerAddressFLASH)+j]);
                                qWarning("Failed verify at address 0x%x", i);
                                IoWithDeviceComplet("Verify EEPROM Memory", Comm::Fail, ((double)elapsed.elapsed()) / 1000);
                                return;
                            }
                        }
                    }
                }
            }//foreach(hexRange, hexData->ranges)
            IoWithDeviceComplet("Verifying", Comm::Success, ((double)elapsed.elapsed()) / 1000);
        }//else if(writeEeprom && (deviceRange.type == EEPROM_MEMORY))
        else if (writeConfig && (deviceRange.type == CONFIG_MEMORY)) {
            elapsed.start();
            IoWithDeviceStart("Verifying Device's Config Words...");

            result = comm->GetData(deviceRange.start,
                                   device->bytesPerPacket,
                                   device->bytesPerAddressConfig,
                                   device->bytesPerWordConfig,
                                   deviceRange.end,
                                   deviceRange.pDataBuffer);

            if (result != Comm::Success) {
                failureDetected = true;
                qWarning("Error reading device.");
                IoWithDeviceComplet("Verifying Device's Config Words", result, ((double)elapsed.elapsed()) / 1000);
            }

            //Search through all of the programmable memory regions from the parsed .hex file data.
            //For each of the programmable memory regions found, if the region also overlaps a region
            //that was included in the device programmed area (which just got read back with GetData()),
            //then verify both the parsed hex contents and read back data match.
            foreach (hexRange, hexData->ranges) {
                if (deviceRange.start == hexRange.start) {
                    //For this entire programmable memory address range, check to see if the data read from the device exactly
                    //matches what was in the hex file.
                    for (i = deviceRange.start; i < deviceRange.end; i++) {
                        //For each byte of each device address (1 on PIC18, 2 on PIC24, since flash memory is 16-bit WORD array)
                        for (j = 0; j < device->bytesPerAddressConfig; j++) {
                            //Compute an index into the device and hex data arrays, based on the current i and j values.
                            arrayIndex = ((i - deviceRange.start) * device->bytesPerAddressConfig)+j;

                            //Check if the device response data matches the data we parsed from the original input .hex file.
                            if (deviceRange.pDataBuffer[arrayIndex] != hexRange.pDataBuffer[arrayIndex]) {
                                //A mismatch was detected.  Perform additional checks to make sure it was a real/unexpected verify failure.

                                //Check if this is a PIC24 device and we are looking at the "phantom byte"
                                //(upper byte [j = 1] of odd address [i%2 == 1] 16-bit flash words).  If the hex data doesn't match
                                //the device (which should be = 0x00 for these locations), this isn't a real verify
                                //failure, since value is a don't care anyway.  This could occur if the hex file imported
                                //doesn't contain all locations, and we "filled" the region with pure 0xFFFFFFFF, instead of 0x00FFFFFF
                                //when parsing the hex file.
                                if((device->family == Device::PIC24) && ((i % 2) == 1) && (j == 1))
                                {
                                    //Not a real verify failure, phantom byte is unimplemented and is a don't care.
                                }//Make further special checks for PIC18 non-J devices
                                else if((device->family == Device::PIC18) && (deviceRange.start == 0x300000) && ((i == 0x300004) || (i == 0x300007)))
                                {
                                     //The "CONFIG3L" and "CONFIG4H" locations (0x300004 and 0x300007) on PIC18 non-J USB devices
                                     //are unimplemented and should be masked out from the verify operation.
                                }
                                else {
                                    //If the data wasn't a match, and this wasn't a PIC24 phantom byte, then if we get
                                    //here this means we found a true verify failure.
                                    failureDetected = true;
                                    if(device->family == Device::PIC24)
                                    {
                                        qWarning("Device: 0x%x Hex: 0x%x", *(unsigned short int*)&deviceRange.pDataBuffer[((i - deviceRange.start) * device->bytesPerAddressConfig)+j], *(unsigned short int*)&hexRange.pDataBuffer[((i - deviceRange.start) * device->bytesPerAddressConfig)+j]);
                                    }
                                    else
                                    {
                                        qWarning("Device: 0x%x Hex: 0x%x", deviceRange.pDataBuffer[((i - deviceRange.start) * device->bytesPerAddressConfig)+j], hexRange.pDataBuffer[((i - deviceRange.start) * device->bytesPerAddressConfig)+j]);
                                    }
                                    qWarning("Failed verify at address 0x%x", i);
                                    IoWithDeviceComplet("Verify Config Bit Memory", Comm::Fail, ((double)elapsed.elapsed()) / 1000);
                                    return;
                                }
                            }
                        }
                    }
                }
            }//foreach(hexRange, hexData->ranges)
            IoWithDeviceComplet("Verifying", Comm::Success, ((double)elapsed.elapsed()) / 1000);
        }//else if(writeConfig && (deviceRange.type == CONFIG_MEMORY))
        else
        {
            continue;
        }
    }//foreach(deviceRange, deviceData->ranges)

    if(failureDetected == false)
    {
        //Successfully verified all regions without error.
        //If this is a v1.01 or later device, we now need to issue the SIGN_FLASH
        //command, and then re-verify the first erase page worth of flash memory
        //(but with the exclusion of the signature WORD address from the verify,
        //since the bootloader firmware will have changed it to the new/magic
        //value (probably 0x600D, or "good" in leet speak).
        if(deviceFirmwareIsAtLeast101 == true)
        {
            comm->SignFlash();
            //Now re-verify the first erase page of flash memory.
            if(device->family == Device::PIC18)
            {
                startOfEraseBlock = extendedBootInfo.PIC18.signatureAddress - (extendedBootInfo.PIC18.signatureAddress % extendedBootInfo.PIC18.erasePageSize);
                result = comm->GetData(startOfEraseBlock,
                                       device->bytesPerPacket,
                                       device->bytesPerAddressFLASH,
                                       device->bytesPerWordFLASH,
                                       (startOfEraseBlock + extendedBootInfo.PIC18.erasePageSize),
                                       &flashData[0]);
                if(result != Comm::Success)
                {
                    failureDetected = true;
                    qWarning("Error reading, post signing, flash data block.");
                }

                //Search through all of the programmable memory regions from the parsed .hex file data.
                //For each of the programmable memory regions found, if the region also overlaps a region
                //that is part of the erase block, copy out bytes into the hexEraseBlockData[] buffer,
                //for re-verification.
                foreach(hexRange, hexData->ranges)
                {
                    //Check if any portion of the range is within the erase block of interest in the device.
                    if((hexRange.start <= startOfEraseBlock) && (hexRange.end > startOfEraseBlock))
                    {
                        unsigned int rangeSize = hexRange.end - hexRange.start;
                        unsigned int address = hexRange.start;
                        unsigned int k = 0;

                        //Check every byte in the hex file range, to see if it is inside the erase block of interest
                        for(i = 0; i < rangeSize; i++)
                        {
                            //Check if the current byte we are looking at is inside the erase block of interst
                            if(((address+i) >= startOfEraseBlock) && ((address+i) < (startOfEraseBlock + extendedBootInfo.PIC18.erasePageSize)))
                            {
                                //The byte is in the erase block of interst.  Copy it out into a new buffer.
                                hexEraseBlockData[k] = *(hexRange.pDataBuffer + i);
                                //Check if this is a signature byte.  If so, replace the value in the buffer
                                //with the post-signing expected signature value, since this is now the expected
                                //value from the device, rather than the value from the hex file...
                                if((address+i) == extendedBootInfo.PIC18.signatureAddress)
                                {
                                    hexEraseBlockData[k] = (unsigned char)extendedBootInfo.PIC18.signatureValue;    //Write LSB of signature into buffer
                                }
                                if((address+i) == (extendedBootInfo.PIC18.signatureAddress + 1))
                                {
                                    hexEraseBlockData[k] = (unsigned char)(extendedBootInfo.PIC18.signatureValue >> 8); //Write MSB into buffer
                                }
                                k++;
                            }
                            if((k >= extendedBootInfo.PIC18.erasePageSize) || (k >= sizeof(hexEraseBlockData)))
                                break;
                        }
                    }
                }//foreach(hexRange, hexData->ranges)

                //We now have both the hex data and the post signing flash erase block data
                //in two RAM buffers.  Compare them to each other to perform post-signing
                //verify.
                for(i = 0; i < extendedBootInfo.PIC18.erasePageSize; i++)
                {
                    if(flashData[i] != hexEraseBlockData[i])
                    {
                        failureDetected = true;
                        qWarning("Post signing verify failure.");
                        EraseDevice();  //Send an erase command, to forcibly
                        //remove the signature (which might be valid), since
                        //there was a verify error and we can't trust the application
                        //firmware image integrity.  This ensures the device jumps
                        //back into bootloader mode always.
                    }
                }
            }

        }//if(deviceFirmwareIsAtLeast101 == true)

    }//if(failureDetected == false)

    if(failureDetected == true) {
        emit writeLog("Operation aborted due to error encountered during verify operation.");
        emit writeLog("Please try the erase/program/verify sequence again.");
        emit writeLog("If repeated failures are encountered, this may indicate the flash");
        emit writeLog("memory has worn out, that the device has been damaged, or that");
        emit writeLog("there is some other unidentified problem.");
    } else {
        IoWithDeviceComplet("Verify", Comm::Success, ((double)elapsed.elapsed()) / 1000);
        emit writeLog("Erase/Program/Verify sequence completed successfully.");
        emit writeLog("You may now unplug or reset the device.");
    }

//    emit SetProgressBar(100);   //Set progress bar to 100%
}//void MainWindow::VerifyDevice()

//This thread programs previously parsed .hex file data into the device's programmable memory regions.
void Bootloader::WriteDevice(void)
{
    QTime elapsed;
    Comm::ErrorCode result;
    DeviceData::MemoryRange hexRange;

    //Update the progress bar so the user knows things are happening.
//    emit SetProgressBar(3);
    //First erase the entire device.
    EraseDevice();

    //Now being re-programming each section based on the info we obtained when
    //we parsed the user's .hex file.

    IoWithDeviceStart("Writing Device...");
    foreach(hexRange, hexData->ranges)
    {
        if(writeFlash && (hexRange.type == PROGRAM_MEMORY))
        {
            //emit IoWithDeviceStarted("Writing Device Program Memory...");
//            elapsed.start();

            result = comm->Program(hexRange.start,
                                   device->bytesPerPacket,
                                   device->bytesPerAddressFLASH,
                                   device->bytesPerWordFLASH,
                                   device->family,
                                   hexRange.end,
                                   hexRange.pDataBuffer);
        }
        else if(writeEeprom && (hexRange.type ==  EEPROM_MEMORY))
        {
                //emit IoWithDeviceStarted("Writing Device EEPROM Memory...");
//                elapsed.start();

                result = comm->Program(hexRange.start,
                                       device->bytesPerPacket,
                                       device->bytesPerAddressEEPROM,
                                       device->bytesPerWordEEPROM,
                                       device->family,
                                       hexRange.end,
                                       hexRange.pDataBuffer);
        }
        else if(writeConfig && (hexRange.type == CONFIG_MEMORY))
        {
            //emit IoWithDeviceStarted("Writing Device Config Words...");
//            elapsed.start();

            result = comm->Program(hexRange.start,
                                   device->bytesPerPacket,
                                   device->bytesPerAddressConfig,
                                   device->bytesPerWordConfig,
                                   device->family,
                                   hexRange.end,
                                   hexRange.pDataBuffer);
        }
        else
        {
            continue;
        }

        IoWithDeviceComplet("Writing", result, ((double)elapsed.elapsed()) / 1000);

        if(result != Comm::Success)
        {
            qWarning("Programming failed");
            return;
        }
    }

    IoWithDeviceComplet("Write", result, ((double)elapsed.elapsed()) / 1000);

    VerifyDevice();
}

void Bootloader::BlankCheckDevice(void)
{
    QTime elapsed;
    Comm::ErrorCode result;
    DeviceData::MemoryRange deviceRange;

    elapsed.start();

    foreach (deviceRange, deviceData->ranges) {
        if (writeFlash && (deviceRange.type == PROGRAM_MEMORY)) {
        IoWithDeviceStart("Blank Checking Device's Program Memory...");

            result = comm->GetData(deviceRange.start,
                                   device->bytesPerPacket,
                                   device->bytesPerAddressFLASH,
                                   device->bytesPerWordFLASH,
                                   deviceRange.end,
                                   deviceRange.pDataBuffer);


            if (result != Comm::Success) {
                qWarning("Blank Check failed");
                IoWithDeviceComplet("Blank Checking Program Memory", result, ((double)elapsed.elapsed()) / 1000);
                return;
            }

            for (unsigned int i = 0; i < ((deviceRange.end - deviceRange.start) * device->bytesPerAddressFLASH); i++) {
                if((deviceRange.pDataBuffer[i] != 0xFF) && !((device->family == Device::PIC24) && ((i % 4) == 3)))
                {
                    qWarning("Failed blank check at address 0x%x", deviceRange.start + i);
                    qWarning("The value was 0x%x", deviceRange.pDataBuffer[i]);
                    IoWithDeviceComplet("Blank Check", Comm::Fail, ((double)elapsed.elapsed()) / 1000);
                    return;
                }
            }
            IoWithDeviceComplet("Blank Checking Program Memory", Comm::Success, ((double)elapsed.elapsed()) / 1000);
        } else if(writeEeprom && deviceRange.type == EEPROM_MEMORY) {
            IoWithDeviceStart("Blank Checking Device's EEPROM Memory...");

            result = comm->GetData(deviceRange.start,
                                   device->bytesPerPacket,
                                   device->bytesPerAddressEEPROM,
                                   device->bytesPerWordEEPROM,
                                   deviceRange.end,
                                   deviceRange.pDataBuffer);


            if (result != Comm::Success) {
                qWarning("Blank Check failed");
                IoWithDeviceComplet("Blank Checking EEPROM Memory", result, ((double)elapsed.elapsed()) / 1000);
                return;
            }
            for (unsigned int i = 0; i < ((deviceRange.end - deviceRange.start) * device->bytesPerWordEEPROM); i++) {
                if(deviceRange.pDataBuffer[i] != 0xFF){
                    qWarning("Failed blank check at address 0x%x + 0x%x", deviceRange.start, i);
                    qWarning("The value was 0x%x", deviceRange.pDataBuffer[i]);
                    IoWithDeviceComplet("Blank Check", Comm::Fail, ((double)elapsed.elapsed()) / 1000);
                    return;
                }
            }
                IoWithDeviceComplet("Blank Checking EEPROM Memory", Comm::Success, ((double)elapsed.elapsed()) / 1000);
        } else {
            continue;
        }
    }
}

void Bootloader::EraseDevice(void)
{
    QTime elapsed;
    Comm::ErrorCode result;
    Comm::BootInfo bootInfo;


    if (writeFlash || writeEeprom) {
        IoWithDeviceStart("Erasing Device... (no status update until complete, may take several seconds)");
        elapsed.start();

        result = comm->Erase();
        if (result != Comm::Success) {
            IoWithDeviceComplet("Erase", result, ((double)elapsed.elapsed()) / 1000);
            return;
        }

        result = comm->ReadBootloaderInfo(&bootInfo);
        IoWithDeviceComplet("Erase", result, ((double)elapsed.elapsed()) / 1000);
    }
}

//Executes when the user clicks the open hex file button on the main form.

void Bootloader::LoadFile(QString newFileName)
{
    QString msg;
    QTextStream stream(&msg);
    QFileInfo nfi(newFileName);

    HexImporter import;
    HexImporter::ErrorCode result;
    Comm::ErrorCode commResultCode;

    hexData->ranges.clear();

    //Print some debug info to the debug window.
    qDebug(QString("Total Programmable Regions Reported by Device: " + QString::number(deviceData->ranges.count(), 10)).toLatin1());

    //First duplicate the deviceData programmable region list and
    //allocate some RAM buffers to hold the hex data that we are about to import.
    foreach (DeviceData::MemoryRange range, deviceData->ranges) {
        //Allocate some RAM for the hex file data we are about to import.
        //Initialize all bytes of the buffer to 0xFF, the default unprogrammed memory value,
        //which is also the "assumed" value, if a value is missing inside the .hex file, but
        //is still included in a programmable memory region.
        range.pDataBuffer = new unsigned char[range.dataBufferLength];
        memset(range.pDataBuffer, 0xFF, range.dataBufferLength);
        hexData->ranges.append(range);

        //Print info regarding the programmable memory region to the debug window.
        qDebug(QString("Device Programmable Region: [" + QString::number(range.start, 16).toUpper() + " - " +
                   QString::number(range.end, 16).toUpper() +")").toLatin1());
    }

    //Import the hex file data into the hexData->ranges[].pDataBuffer buffers.
    result = import.ImportHexFile(newFileName, hexData, device);
    //Based on the result of the hex file import operation, decide how to proceed.
    switch (result) {
        case HexImporter::Success:
            break;

        case HexImporter::CouldNotOpenFile:
            stream << "Error: Could not open file " << nfi.fileName() << "\n";
            emit writeLog(msg);
            return;

        case HexImporter::NoneInRange:
            stream << "No address within range in file: " << nfi.fileName() << ".  Verify the correct firmware image was specified and is designed for your device.\n";
            emit writeLog(msg);
            return;

        case HexImporter::ErrorInHexFile:
            stream << "Error in hex file.  Please make sure the firmware image supplied was designed for the device to be programmed. \n";
            emit writeLog(msg);
            return;
        case HexImporter::InsufficientMemory:
            stream << "Memory allocation failed.  Please close other applications to free up system RAM and try again. \n";
            emit writeLog(msg);
            return;

        default:
            stream << "Failed to import: " << result << "\n";
            emit writeLog(msg);
            return;
    }

    //Check if the user has imported a .hex file that doesn't contain config bits in it,
    //even though the user is planning on re-programming the config bits section.
    if (writeConfig && (import.hasConfigBits == false) && device->hasConfig()) {
        //The user had config bit reprogramming selected, but the hex file opened didn't have config bit
        //data in it.  We should automatically prevent config bit programming, to avoid leaving the device
        //in a broken state following the programming cycle.
        commResultCode = comm->LockUnlockConfig(true); //Lock the config bits.
        if(commResultCode != Comm::Success) {
            emit writeLog("Unexpected internal error encountered.  Recommend restarting the application to avoid ""bricking"" the device.\n");
        }

//???        QMessageBox::warning(this, "Warning!", "This HEX file does not contain config bit information.\n\nAutomatically disabling config bit reprogramming to avoid leaving the device in a state that could prevent further bootloading.", QMessageBox::AcceptRole, QMessageBox::AcceptRole);
        writeConfig = false;
    }

    fileName = newFileName;
    watchFileName = newFileName;

    hexOpen = true;

    return;
}

void Bootloader::GetQuery() {
    QTime totalTime;
    Comm::BootInfo bootInfo;
    DeviceData::MemoryRange range;
    QString connectMsg;
    QTextStream ss(&connectMsg);


    qDebug("Executing GetQuery() command.");

    totalTime.start();

    if(!comm->isConnected()){
        qWarning("Query not sent, device not connected");
        return;
    }

    //Send the Query command to the device over USB, and check the result status.
    switch(comm->ReadBootloaderInfo(&bootInfo))
    {
        case Comm::Fail:
        case Comm::IncorrectCommand:
        emit writeLog("Unable to communicate with device\n");
            return;
        case Comm::Timeout:
            ss << "Operation timed out";
            break;
        case Comm::Success:
            wasBootloaderMode = true;
            ss << "Device Ready";
            emit writeLog("Connected");
            break;
        default:
            return;
    }

    ss << " (" << (double)totalTime.elapsed() / 1000 << "s)\n";
    emit writeLog(connectMsg);
    deviceData->ranges.clear();

    //Now start parsing the bootInfo packet to learn more about the device.  The bootInfo packet contains
    //contains the query response data from the USB device.  We will save these values into globabl variables
    //so other parts of the application can use the info when deciding how to do things.
    device->family = (Device::Families) bootInfo.deviceFamily;
    device->bytesPerPacket = bootInfo.bytesPerPacket;

    //Set some processor family specific global variables that will be used elsewhere (ex: during program/verify operations).
    switch(device->family){
        case Device::PIC18:
            device->bytesPerWordFLASH = 2;
            device->bytesPerAddressFLASH = 1;
            break;
        case Device::PIC24:
            device->bytesPerWordFLASH = 4;
            device->bytesPerAddressFLASH = 2;
            device->bytesPerWordConfig = 4;
            device->bytesPerAddressConfig = 2;
            break;
        case Device::PIC32:
            device->bytesPerWordFLASH = 4;
            device->bytesPerAddressFLASH = 1;
            break;
        case Device::PIC16:
            device->bytesPerWordFLASH = 2;
            device->bytesPerAddressFLASH = 2;
        default:
            device->bytesPerWordFLASH = 2;
            device->bytesPerAddressFLASH = 1;
            break;
    }

    //Initialize the deviceData buffers and length variables, with the regions that the firmware claims are
    //reprogrammable.  We will need this information later, to decide what part(s) of the .hex file we
    //should look at/try to program into the device.  Data sections in the .hex file that are not included
    //in these regions should be ignored.
    for (int i = 0; i < MAX_DATA_REGIONS; i++) {
        if (bootInfo.memoryRegions[i].type == END_OF_TYPES_LIST) {
            //Before we quit, check the special versionFlag byte,
            //to see if the bootloader firmware is at least version 1.01.
            //If it is, then it will support the extended query command.
            //If the device is based on v1.00 bootloader firmware, it will have
            //loaded the versionFlag location with 0x00, which was a pad byte.
            if (bootInfo.versionFlag == BOOTLOADER_V1_01_OR_NEWER_FLAG) {
                deviceFirmwareIsAtLeast101 = true;
                qDebug("Device bootloader firmware is v1.01 or newer and supports Extended Query.");
                //Now fetch the extended query information packet from the USB firmware.
                comm->ReadExtendedQueryInfo(&extendedBootInfo);
                qDebug("Device bootloader firmware version is: " + extendedBootInfo.PIC18.bootloaderVersion);
            } else {
                deviceFirmwareIsAtLeast101 = false;
            }
            break;
        }

        //Error check: Check the firmware's reported size to make sure it is sensible.  This ensures
        //we don't try to allocate ourselves a massive amount of RAM (capable of crashing this PC app)
        //if the firmware claimed an improper value.
        if (bootInfo.memoryRegions[i].size > MAXIMUM_PROGRAMMABLE_MEMORY_SEGMENT_SIZE) {
            bootInfo.memoryRegions[i].size = MAXIMUM_PROGRAMMABLE_MEMORY_SEGMENT_SIZE;
        }

        //Parse the bootInfo response packet and allocate ourselves some RAM to hold the eventual data to program.
        if (bootInfo.memoryRegions[i].type == PROGRAM_MEMORY) {
            range.type = PROGRAM_MEMORY;
            range.dataBufferLength = bootInfo.memoryRegions[i].size * device->bytesPerAddressFLASH;
            range.pDataBuffer = new unsigned char[range.dataBufferLength];
            memset(&range.pDataBuffer[0], 0xFF, range.dataBufferLength);
        } else if(bootInfo.memoryRegions[i].type == EEPROM_MEMORY) {
            range.type = EEPROM_MEMORY;
            range.dataBufferLength = bootInfo.memoryRegions[i].size * device->bytesPerAddressEEPROM;
            range.pDataBuffer = new unsigned char[range.dataBufferLength];
            memset(&range.pDataBuffer[0], 0xFF, range.dataBufferLength);
        } else if(bootInfo.memoryRegions[i].type == CONFIG_MEMORY) {
            range.type = CONFIG_MEMORY;
            range.dataBufferLength = bootInfo.memoryRegions[i].size * device->bytesPerAddressConfig;
            range.pDataBuffer = new unsigned char[range.dataBufferLength];
            memset(&range.pDataBuffer[0], 0xFF, range.dataBufferLength);
        }

        //Notes regarding range.start and range.end: The range.start is defined as the starting address inside
        //the USB device that will get programmed.  For example, if the bootloader occupies 0x000-0xFFF flash
        //memory addresses (ex: on a PIC18), then the starting bootloader programmable address would typically
        //be = 0x1000 (ex: range.start = 0x1000).
        //The range.end is defined as the last address that actually gets programmed, plus one, in this programmable
        //region.  For example, for a 64kB PIC18 microcontroller, the last implemented flash memory address
        //is 0xFFFF.  If the last 1024 bytes are reserved by the bootloader (since that last page contains the config
        //bits for instance), then the bootloader firmware may only allow the last address to be programmed to
        //be = 0xFBFF.  In this scenario, the range.end value would be = 0xFBFF + 1 = 0xFC00.
        //When this application uses the range.end value, it should be aware that the actual address limit of
        //range.end does not actually get programmed into the device, but the address just below it does.
        //In this example, the programmed region would end up being 0x1000-0xFBFF (even though range.end = 0xFC00).
        //The proper code to program this would basically be something like this:
        //for(i = range.start; i < range.end; i++)
        //{
        //    //Insert code here that progams one device address.  Note: for PIC18 this will be one byte for flash memory.
        //    //For PIC24 this is actually 2 bytes, since the flash memory is addressed as a 16-bit word array.
        //}
        //In the above example, the for() loop exits just before the actual range.end value itself is programmed.

        range.start = bootInfo.memoryRegions[i].address;
        range.end = bootInfo.memoryRegions[i].address + bootInfo.memoryRegions[i].size;
        //Add the new structure+buffer to the list
        deviceData->ranges.append(range);
    }
}

