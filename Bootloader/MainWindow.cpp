/************************************************************************
* Copyright (c) 2009-2011,  Microchip Technology Inc.
*
* Microchip licenses this software to you solely for use with Microchip
* products.  The software is owned by Microchip and its licensors, and
* is protected under applicable copyright laws.  All rights reserved.
*
* SOFTWARE IS PROVIDED "AS IS."  MICROCHIP EXPRESSLY DISCLAIMS ANY
* WARRANTY OF ANY KIND, WHETHER EXPRESS OR IMPLIED, INCLUDING BUT
* NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
* FOR A PARTICULAR PURPOSE, OR NON-INFRINGEMENT.  IN NO EVENT SHALL
* MICROCHIP BE LIABLE FOR ANY INCIDENTAL, SPECIAL, INDIRECT OR
* CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, HARM TO YOUR
* EQUIPMENT, COST OF PROCUREMENT OF SUBSTITUTE GOODS, TECHNOLOGY
* OR SERVICES, ANY CLAIMS BY THIRD PARTIES (INCLUDING BUT NOT LIMITED
* TO ANY DEFENSE THEREOF), ANY CLAIMS FOR INDEMNITY OR CONTRIBUTION,
* OR OTHER SIMILAR COSTS.
*
* To the fullest extent allowed by law, Microchip and its licensors
* liability shall not exceed the amount of fees, if any, that you
* have paid directly to Microchip to use this software.
*
* MICROCHIP PROVIDES THIS SOFTWARE CONDITIONALLY UPON YOUR ACCEPTANCE
* OF THESE TERMS.
*
* Author        Date        Ver   Comment
*************************************************************************
* E. Schlunder  2009/04/14  0.01  Initial code ported from VB app.
* T. Lawrence   2011/01/14  2.90  Initial implementation of USB version of this
*                                 bootloader application.
* F. Schlunder  2011/07/06  2.90a Small update to support importing of hex files
*                                 with "non-monotonic" line address ordering.
************************************************************************/

#include <QTextStream>
#include <QByteArray>
#include <QList>
#include <QTime>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QSettings>
#include <QtWidgets/QDesktopWidget>
#include <QtConcurrent/QtConcurrentRun>

#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "Settings.h"

#include "version.h"



//Surely the micro doesn't have a programmable memory region greater than 268 Megabytes...
//Value used for error checking device reponse values.
#define MAXIMUM_PROGRAMMABLE_MEMORY_SEGMENT_SIZE 0x0FFFFFFF

bool deviceFirmwareIsAtLeast101 = false;
Comm::ExtendedQueryInfo extendedBootInfo;

void MainWindow::writeLog(Qstring value){
    ui->plainTextEdit->setPlainText(value);
}


MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindowClass)
{
    mBootloader = new Bootloader();
    QObject::connect(&mBootloader, SIGNAL(writeLog(QString)),&this, SLOT(writeLog(QStrign)));




    int i;
    mBootloader->hexOpen = false;
    fileWatcher = NULL;
    timer = new QTimer();

    ui->setupUi(this);
    setWindowTitle(APPLICATION + QString(" v") + VERSION);

    QSettings settings;
    settings.beginGroup("MainWindow");
    fileName = settings.value("fileName").toString();

    for(i = 0; i < MAX_RECENT_FILES; i++)
    {
        recentFiles[i] = new QAction(this);
        connect(recentFiles[i], SIGNAL(triggered()), this, SLOT(openRecentFile()));
        recentFiles[i]->setVisible(false);
        ui->menuFile->insertAction(ui->actionExit, recentFiles[i]);
    }
    ui->menuFile->insertSeparator(ui->actionExit);

    settings.endGroup();

    settings.beginGroup("WriteOptions");
    mBootloader->writeFlash = settings.value("writeFlash", true).toBool();
    //writeConfig = settings.value("writeConfig", false).toBool();
    mBootloader->writeConfig = false; //Force user to manually re-enable it every time they re-launch the application.  Safer that way.
    mBootloader->writeEeprom = settings.value("writeEeprom", false).toBool();
    mBootloader->eraseDuringWrite = true;
    settings.endGroup();

    qRegisterMetaType<Comm::ErrorCode>("Comm::ErrorCode");

    connect(timer, SIGNAL(timeout()), this, SLOT(Connection()));
    connect(this, SIGNAL(IoWithDeviceCompleted(QString,Comm::ErrorCode,double)), this, SLOT(IoWithDeviceComplete(QString,Comm::ErrorCode,double)));
    connect(this, SIGNAL(IoWithDeviceStarted(QString)), this, SLOT(IoWithDeviceStart(QString)));
    connect(this, SIGNAL(AppendString(QString)), this, SLOT(AppendStringToTextbox(QString)));
    connect(this, SIGNAL(SetProgressBar(int)), this, SLOT(UpdateProgressBar(int)));
    connect(mBootloader->comm, SIGNAL(SetProgressBar(int)), this, SLOT(UpdateProgressBar(int)));


    this->statusBar()->addPermanentWidget(&deviceLabel);
    deviceLabel.setText("Disconnected");

    //Make initial check to see if the USB device is attached
    mBootloader->comm->PollUSB();
    if(mBootloader->comm->isConnected())
    {
        qWarning("Attempting to open device...");
        mBootloader->comm->open();
        ui->plainTextEdit->setPlainText("Device Attached.");
        ui->plainTextEdit->appendPlainText("Connecting...");
        mBootloader->GetQuery();
    }
    else
    {
        ui->plainTextEdit->appendPlainText("Device not detected.  Verify device is attached and in firmware update mode.");
        deviceLabel.setText("Disconnected");
        mBootloader->hexOpen = false;
        setBootloadEnabled(false);
        emit SetProgressBar(0);
    }

    //Update the file list in the File-->[import files list] area, so the user can quickly re-load a previously used .hex file.
    UpdateRecentFileList();


    timer->start(1000); //Check for future USB connection status changes every 1000 milliseconds.
}

MainWindow::~MainWindow()
{
    QSettings settings;

    settings.beginGroup("MainWindow");
    settings.setValue("size", size());
    settings.setValue("pos", pos());
    settings.setValue("fileName", fileName);
    settings.endGroup();

    settings.beginGroup("WriteOptions");
    settings.setValue("writeFlash", mBootloader->writeFlash);
    settings.setValue("writeConfig", mBootloader->writeConfig);
    settings.setValue("writeEeprom", mBootloader->writeEeprom);
    settings.endGroup();

    mBootloader->comm->close();
    setBootloadEnabled(false);

    delete timer;
    delete ui;
    delete mBootloader;
}

void MainWindow::Connection(void)
{
    bool currStatus = mBootloader->comm->isConnected();
    Comm::ErrorCode result;

    mBootloader->comm->PollUSB();

    if(currStatus != mBootloader->comm->isConnected())
    {
        UpdateRecentFileList();

        if(mBootloader->comm->isConnected())
        {
            qWarning("Attempting to open device...");
            mBootloader->comm->open();
            ui->plainTextEdit->setPlainText("Device Attached.");
            ui->plainTextEdit->appendPlainText("Connecting...");
            if(mBootloader->writeConfig)
            {
                //ui->plainTextEdit->appendPlainText("Disabling Erase button to prevent accidental erasing of the configuration words without reprogramming them\n");
                mBootloader->writeConfig = true;
                result = mBootloader->comm->LockUnlockConfig(false);
                if(result == Comm::Success)
                {
                    ui->plainTextEdit->appendPlainText("Unlocked Configuration bits successfully\n");
                }
            }
            else
            {
                mBootloader->writeConfig = false;
            }
            mBootloader->GetQuery();
        }
        else
        {
            qWarning("Closing device.");
            mBootloader->comm->close();
            deviceLabel.setText("Disconnected");
            ui->plainTextEdit->setPlainText("Device Detached.");
            mBootloader->hexOpen = false;
            setBootloadEnabled(false);
            emit SetProgressBar(0);
        }
    }
}

void MainWindow::setBootloadEnabled(bool enable)
{
    ui->action_Settings->setEnabled(enable);
    ui->actionErase_Device->setEnabled(enable && !mBootloader->writeConfig);
    ui->actionWrite_Device->setEnabled(enable && mBootloader->hexOpen);
    ui->actionExit->setEnabled(enable);
    ui->action_Verify_Device->setEnabled(enable && mBootloader->hexOpen);
    ui->actionOpen->setEnabled(enable);
    ui->actionBlank_Check->setEnabled(enable && !mBootloader->writeConfig);
    ui->actionReset_Device->setEnabled(enable);
}

void MainWindow::setBootloadBusy(bool busy)
{
    if(busy)
    {
        QApplication::setOverrideCursor(Qt::BusyCursor);
        timer->stop();
    }
    else
    {
        QApplication::restoreOverrideCursor();
        timer->start(1000);
    }

    ui->action_Settings->setEnabled(!busy);
    ui->actionErase_Device->setEnabled(!busy && !mBootloader->writeConfig);
    ui->actionWrite_Device->setEnabled(!busy && mBootloader->hexOpen);
    ui->actionExit->setEnabled(!busy);
    ui->action_Verify_Device->setEnabled(!busy && mBootloader->hexOpen);
    ui->actionOpen->setEnabled(!busy);
    ui->action_Settings->setEnabled(!busy);
    ui->actionBlank_Check->setEnabled(!busy && !mBootloader->writeConfig);
    ui->actionReset_Device->setEnabled(!busy);
}

void MainWindow::on_actionExit_triggered()
{
    QApplication::exit();
}

void MainWindow::IoWithDeviceStart(QString msg)
{
    ui->plainTextEdit->appendPlainText(msg);
    setBootloadBusy(true);
}


//Useful for adding lines of text to the main window from other threads.
void MainWindow::AppendStringToTextbox(QString msg)
{
    ui->plainTextEdit->appendPlainText(msg);
}

void MainWindow::UpdateProgressBar(int newValue)
{
    ui->progressBar->setValue(newValue);
}



void MainWindow::IoWithDeviceComplete(QString msg, Comm::ErrorCode result, double time)
{
    QTextStream ss(&msg);

    switch(result)
    {
        case Comm::Success:
            ss << " Complete (" << time << "s)\n";
            setBootloadBusy(false);
            break;
        case Comm::NotConnected:
            ss << " Failed. Device not connected.\n";
            setBootloadBusy(false);
            break;
        case Comm::Fail:
            ss << " Failed.\n";
            setBootloadBusy(false);
            break;
        case Comm::IncorrectCommand:
            ss << " Failed. Unable to communicate with device.\n";
            setBootloadBusy(false);
            break;
        case Comm::Timeout:
            ss << " Timed out waiting for device (" << time << "s)\n";
            setBootloadBusy(false);
            break;
        default:
            break;
    }

    ui->plainTextEdit->appendPlainText(msg);
}

void MainWindow::on_action_Verify_Device_triggered()
{
    future = QtConcurrent::run(mBootloader, &Bootloader::VerifyDevice);
}



//Gets called when the user clicks to program button in the GUI.
void MainWindow::on_actionWrite_Device_triggered()
{
    future = QtConcurrent::run(mBootloader, &Bootloader::WriteDevice);
    ui->plainTextEdit->clear();
    ui->plainTextEdit->appendPlainText("Starting Erase/Program/Verify Sequence.");
    ui->plainTextEdit->appendPlainText("Do not unplug device or disconnect power until the operation is fully complete.");
    ui->plainTextEdit->appendPlainText(" ");
}



void MainWindow::on_actionBlank_Check_triggered()
{
    future = QtConcurrent::run(mBootloader, &Bootloader::BlankCheckDevice);
}

void MainWindow::on_actionErase_Device_triggered()
{
    future = QtConcurrent::run(mBootloader, &Bootloader::EraseDevice);
}

//Executes when the user clicks the open hex file button on the main form.
void MainWindow::on_actionOpen_triggered()
{
    QString msg, newFileName;
    QTextStream stream(&msg);

    //Create an open file dialog box, so the user can select a .hex file.
    newFileName =
        QFileDialog::getOpenFileName(this, "Open Hex File", fileName, "Hex Files (*.hex *.ehx)");

    if(newFileName.isEmpty())
    {
        return;
    }

    LoadFile(newFileName);
}

void MainWindow::LoadFile(QString newFileName)
{
    QString msg;
    QTextStream stream(&msg);
    QFileInfo nfi(newFileName);

    QApplication::setOverrideCursor(Qt::BusyCursor);

    HexImporter import;
    HexImporter::ErrorCode result;
    Comm::ErrorCode commResultCode;

    mBootloader->hexData->ranges.clear();

    //Print some debug info to the debug window.
    qDebug(QString("Total Programmable Regions Reported by Device: " + QString::number(mBootloader->deviceData->ranges.count(), 10)).toLatin1());

    //First duplicate the deviceData programmable region list and
    //allocate some RAM buffers to hold the hex data that we are about to import.
    foreach(DeviceData::MemoryRange range, mBootloader->deviceData->ranges)
    {
        //Allocate some RAM for the hex file data we are about to import.
        //Initialize all bytes of the buffer to 0xFF, the default unprogrammed memory value,
        //which is also the "assumed" value, if a value is missing inside the .hex file, but
        //is still included in a programmable memory region.
        range.pDataBuffer = new unsigned char[range.dataBufferLength];
        memset(range.pDataBuffer, 0xFF, range.dataBufferLength);
        mBootloader->hexData->ranges.append(range);

        //Print info regarding the programmable memory region to the debug window.
        qDebug(QString("Device Programmable Region: [" + QString::number(range.start, 16).toUpper() + " - " +
                   QString::number(range.end, 16).toUpper() +")").toLatin1());
    }

    //Import the hex file data into the hexData->ranges[].pDataBuffer buffers.
    result = import.ImportHexFile(newFileName, mBootloader->hexData, mBootloader->device);
    //Based on the result of the hex file import operation, decide how to proceed.
    switch(result)
    {
        case HexImporter::Success:
            break;

        case HexImporter::CouldNotOpenFile:
            QApplication::restoreOverrideCursor();
            stream << "Error: Could not open file " << nfi.fileName() << "\n";
            ui->plainTextEdit->appendPlainText(msg);
            return;

        case HexImporter::NoneInRange:
            QApplication::restoreOverrideCursor();
            stream << "No address within range in file: " << nfi.fileName() << ".  Verify the correct firmware image was specified and is designed for your device.\n";
            ui->plainTextEdit->appendPlainText(msg);
            return;

        case HexImporter::ErrorInHexFile:
            QApplication::restoreOverrideCursor();
            stream << "Error in hex file.  Please make sure the firmware image supplied was designed for the device to be programmed. \n";
            ui->plainTextEdit->appendPlainText(msg);
            return;
        case HexImporter::InsufficientMemory:
            QApplication::restoreOverrideCursor();
            stream << "Memory allocation failed.  Please close other applications to free up system RAM and try again. \n";
            ui->plainTextEdit->appendPlainText(msg);
            return;

        default:
            QApplication::restoreOverrideCursor();
            stream << "Failed to import: " << result << "\n";
            ui->plainTextEdit->appendPlainText(msg);
            return;
    }

    //Check if the user has imported a .hex file that doesn't contain config bits in it,
    //even though the user is planning on re-programming the config bits section.
    if(mBootloader->writeConfig && (import.hasConfigBits == false) && mBootloader->device->hasConfig())
    {
        //The user had config bit reprogramming selected, but the hex file opened didn't have config bit
        //data in it.  We should automatically prevent config bit programming, to avoid leaving the device
        //in a broken state following the programming cycle.
        commResultCode = mBootloader->comm->LockUnlockConfig(true); //Lock the config bits.
        if(commResultCode != Comm::Success)
        {
            ui->plainTextEdit->appendPlainText("Unexpected internal error encountered.  Recommend restarting the application to avoid ""bricking"" the device.\n");
        }

        QMessageBox::warning(this, "Warning!", "This HEX file does not contain config bit information.\n\nAutomatically disabling config bit reprogramming to avoid leaving the device in a state that could prevent further bootloading.", QMessageBox::AcceptRole, QMessageBox::AcceptRole);
        mBootloader->writeConfig = false;
    }

    fileName = newFileName;
    watchFileName = newFileName;

    QSettings settings;
    settings.beginGroup("MainWindow");

    QStringList files = settings.value("recentFileList").toStringList();
    files.removeAll(fileName);
    files.prepend(fileName);
    while(files.size() > MAX_RECENT_FILES)
    {
        files.removeLast();
    }
    settings.setValue("recentFileList", files);
    UpdateRecentFileList();

    stream.setIntegerBase(10);

    msg.clear();
    QFileInfo fi(fileName);
    QString name = fi.fileName();
    stream << "Opened: " << name << "\n";
    ui->plainTextEdit->appendPlainText(msg);
    mBootloader->hexOpen = true;
    setBootloadEnabled(true);
    QApplication::restoreOverrideCursor();

    return;
}

void MainWindow::openRecentFile(void)
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (action)
    {
        LoadFile(action->data().toString());
    }
}

void MainWindow::UpdateRecentFileList(void)
{
    QSettings settings;
    settings.beginGroup("MainWindow");
    QStringList files;

    files = settings.value("recentFileList").toStringList();

    int recentFileCount = qMin(files.size(), MAX_RECENT_FILES);
    QString text;
    int i;

    for(i = 0; i < recentFileCount; i++)
    {
        text = tr("&%1 %2").arg(i + 1).arg(QFileInfo(files[i]).fileName());

        recentFiles[i]->setText(text);
        recentFiles[i]->setData(files[i]);
        recentFiles[i]->setVisible(mBootloader->comm->isConnected());
    }

    for(; i < MAX_RECENT_FILES; i++)
    {
        recentFiles[i]->setVisible(false);
    }
}

void MainWindow::on_action_About_triggered()
{
    QString msg;
    QTextStream stream(&msg);

    stream << "USB HID Bootloader v" << VERSION << "\n";
    stream << "Copyright " << (char)Qt::Key_copyright << " 2011-2013,  Microchip Technology Inc.\n\n";

    stream << "Microchip licenses this software to you solely for use with\n";
    stream << "Microchip products. The software is owned by Microchip and\n";
    stream << "its licensors, and is protected under applicable copyright\n";
    stream << "laws. All rights reserved.\n\n";

    stream << "SOFTWARE IS PROVIDED \"AS IS.\"  MICROCHIP EXPRESSLY\n";
    stream << "DISCLAIMS ANY WARRANTY OF ANY KIND, WHETHER EXPRESS\n";
    stream << "OR IMPLIED, INCLUDING BUT NOT LIMITED TO, THE IMPLIED\n";
    stream << "WARRANTIES OF MERCHANTABILITY, FITNESS FOR A\n";
    stream << "PARTICULAR PURPOSE, OR NON-INFRINGEMENT.  IN NO EVENT\n";
    stream << "SHALL MICROCHIP BE LIABLE FOR ANY INCIDENTAL, SPECIAL,\n";
    stream << "INDIRECT OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR\n";
    stream << "LOST DATA, HARM TO YOUR EQUIPMENT, COST OF\n";
    stream << "PROCUREMENT OF SUBSTITUTE GOODS, TECHNOLOGY OR\n";
    stream << "SERVICES, ANY CLAIMS BY THIRD PARTIES (INCLUDING BUT\n";
    stream << "NOT LIMITED TO ANY DEFENSE THEREOF), ANY CLAIMS FOR\n";
    stream << "INDEMNITY OR CONTRIBUTION, OR OTHER SIMILAR COSTS.\n\n";

    stream << "To the fullest extent allowed by law, Microchip and its\n";
    stream << "licensors liability shall not exceed the amount of fees, if any,\n";
    stream << "that you have paid directly to Microchip to use this software.\n\n";

    stream << "MICROCHIP PROVIDES THIS SOFTWARE CONDITIONALLY UPON\n";
    stream << "YOUR ACCEPTANCE OF THESE TERMS.";

    QMessageBox::about(this, "About", msg);
}

void MainWindow::on_action_Settings_triggered()
{
    Comm::ErrorCode result;
    Settings* dlg = new Settings(this);

    dlg->enableEepromBox(mBootloader->device->hasEeprom());

    dlg->setWriteFlash(mBootloader->writeFlash);
    dlg->setWriteConfig(mBootloader->writeConfig);
    dlg->setWriteEeprom(mBootloader->writeEeprom);

    if(dlg->exec() == QDialog::Accepted)
    {
        mBootloader->writeFlash = dlg->writeFlash;
        mBootloader->writeEeprom = dlg->writeEeprom;

        if(!mBootloader->writeConfig && dlg->writeConfig)
        {
            ui->plainTextEdit->appendPlainText("Disabling Erase button to prevent accidental erasing of the configuration words without reprogramming them\n");
            mBootloader->writeConfig = true;
            mBootloader->hexOpen = false;
            result = mBootloader->comm->LockUnlockConfig(false);
            if(result == Comm::Success)
            {
                ui->plainTextEdit->appendPlainText("Unlocked Configuration bits successfully\n");
                mBootloader->GetQuery();
            }
        }
        else if(mBootloader->writeConfig && !dlg->writeConfig)
        {
            mBootloader->writeConfig = false;
            mBootloader->hexOpen = false;
            result = mBootloader->comm->LockUnlockConfig(true);
            if(result == Comm::Success)
            {
                ui->plainTextEdit->appendPlainText("Locked Configuration bits successfully\n");
                mBootloader->GetQuery();
            }
        }

        if(!(mBootloader->writeFlash || mBootloader->writeEeprom || mBootloader->writeConfig))
        {
            setBootloadEnabled(false);
            ui->action_Settings->setEnabled(true);
        }
        else
        {
            setBootloadEnabled(true);
        }
    }

    delete dlg;
}

void MainWindow::on_actionReset_Device_triggered()
{
    if(!mBootloader->comm->isConnected())
    {
        failed = -1;
        qWarning("Reset not sent, device not connected");
        return;
    }

    ui->plainTextEdit->appendPlainText("Resetting...");
    mBootloader->comm->Reset();
}
