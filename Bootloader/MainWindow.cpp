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
//#include <QByteArray>
//#include <QList>
//#include <QTime>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QSettings>
#include <QtWidgets/QDesktopWidget>
#include <QtConcurrent/QtConcurrentRun>

#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "Settings.h"

#include "version.h"


MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindowClass) {
    ui->setupUi(this);
    setWindowTitle(APPLICATION + QString(" v") + VERSION);
    this->statusBar()->addPermanentWidget(&deviceLabel);

    labelMemoryRanges.setText("Memory Range: ");
    comboMemoryRanges.setMinimumWidth(200);
    ui->mainToolBar->addWidget(&labelMemoryRanges);
    ui->mainToolBar->addWidget(&comboMemoryRanges);
    comboMemoryRanges.addItem("All reanges",QVariant(0));
    connect(&comboMemoryRanges , SIGNAL(currentIndexChanged(int)),this,SLOT(onChangedMemoryRanges(int)));

    qRegisterMetaType<Comm::ErrorCode>("Comm::ErrorCode");
    qRegisterMetaType<Bootloader::MessageType>("Bootloader::MessageType");


    bootloader = new Bootloader();

    connect(bootloader, SIGNAL(setConnected(bool)), this, SLOT(setConnected(bool)));
    connect(bootloader, SIGNAL(setBootloadEnabled(bool)), this, SLOT(setBootloadEnabled(bool)));
    connect(bootloader, SIGNAL(setBootloadBusy(bool)), this, SLOT(setBootloadBusy(bool)));
    connect(bootloader, SIGNAL(setProgressBar(int)), this, SLOT(updateProgressBar(int)));
    connect(bootloader, SIGNAL(message(Bootloader::MessageType, QString)), this, SLOT(onMessage(Bootloader::MessageType, QString)));
    connect(bootloader, SIGNAL(messageClear()), this, SLOT(onMessageClear()));


    fileWatcher = NULL;

    QSettings settings;
    settings.beginGroup("MainWindow");
    fileName = settings.value("fileName").toString();

    int i;
    for (i = 0; i < MAX_RECENT_FILES; i++) {
        recentFiles[i] = new QAction(this);
        connect(recentFiles[i], SIGNAL(triggered()), this, SLOT(openRecentFile()));
        recentFiles[i]->setVisible(false);
        ui->menuFile->insertAction(ui->actionExit, recentFiles[i]);
    }
    ui->menuFile->insertSeparator(ui->actionExit);

    settings.endGroup();

    settings.beginGroup("WriteOptions");
    bootloader->writeFlash = settings.value("writeFlash", true).toBool();
    //writeConfig = settings.value("writeConfig", false).toBool();
    bootloader->writeConfig = false; //Force user to manually re-enable it every time they re-launch the application.  Safer that way.
    bootloader->writeEeprom = settings.value("writeEeprom", false).toBool();
    bootloader->eraseDuringWrite = true;
    settings.endGroup();

    //    //Make initial check to see if the USB device is attached
    //    bootloader->comm->PollUSB();
    //    if(bootloader->comm->isConnected())
    //    {
    //        qWarning("Attempting to open device...");
    //        bootloader->comm->open();
    //        ui->plainTextEdit->setPlainText("Device Attached.");
    //        ui->plainTextEdit->appendPlainText("Connecting...");
    //        bootloader->GetQuery();
    //    }
    //    else
    //    {
    //        ui->plainTextEdit->appendPlainText("Device not detected.  Verify device is attached and in firmware update mode.");
    //        deviceLabel.setText("Disconnected");
    //        bootloader->hexOpen = false;
    //        setBootloadEnabled(false);
    //        emit SetProgressBar(0);
    //    }
    // Disconnected by default, if device attached bootloader will update UI in 1 sec
    setConnected(false);
    setBootloadEnabled(false);

    //Update the file list in the File-->[import files list] area, so the user can quickly re-load a previously used .hex file.
    //UpdateRecentFileList(); // moved to setConnected()
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
    settings.setValue("writeFlash", bootloader->writeFlash);
    settings.setValue("writeConfig", bootloader->writeConfig);
    settings.setValue("writeEeprom", bootloader->writeEeprom);
    settings.endGroup();

    setBootloadEnabled(false);

    delete ui;
    delete bootloader;
}

void MainWindow::onChangedMemoryRanges(int value)
{
    bootloader->rangeReadWrite = comboMemoryRanges.itemData(value).toInt();
}

void MainWindow::setConnected(bool connected)
{
    if (connected) {
        deviceLabel.setText("Connected");

        // Ranges
        comboMemoryRanges.clear();
        comboMemoryRanges.addItem(bootloader->GetMemoryRangeNameByType(ALL_MEMORY_RANGES),QVariant(ALL_MEMORY_RANGES));
        for (int i = 0; i < bootloader->deviceData->ranges.size(); i++) {
           QString memoryRangeName = bootloader->GetMemoryRangeNameByType(bootloader->deviceData->ranges[i].type);
           QString memoryRangeStartHex =  QString::number(bootloader->deviceData->ranges[i].start, 16).toUpper();
           QString memoryRangeEndHex = QString::number(bootloader->deviceData->ranges[i].end, 16).toUpper();

           QString formattedString = QString("[0x%1]-[0x%2] %3").arg(memoryRangeStartHex).arg(memoryRangeEndHex).arg(memoryRangeName);
           comboMemoryRanges.addItem(formattedString, QVariant(i));
        }
    } else {
        deviceLabel.setText("Disconnected");
        comboMemoryRanges.clear();
        comboMemoryRanges.addItem(bootloader->GetMemoryRangeNameByType(ALL_MEMORY_RANGES),QVariant(ALL_MEMORY_RANGES));
    }
    ui->action_Settings->setEnabled(connected);

    // Was in original project, looks like because calling from constructor does not work
    UpdateRecentFileList();
}

void MainWindow::setBootloadEnabled(bool enable)
{
    //ui->action_Settings->setEnabled(enable);
    ui->actionErase_Device->setEnabled(enable && !bootloader->writeConfig);
    ui->actionWrite_Device->setEnabled(enable && bootloader->hexOpen);
    ui->actionExit->setEnabled(enable);
    ui->action_Verify_Device->setEnabled(enable && bootloader->hexOpen);
    ui->actionOpen->setEnabled(enable);
    ui->actionBlank_Check->setEnabled(enable && !bootloader->writeConfig);
    ui->actionReset_Device->setEnabled(enable);

    comboMemoryRanges.setEnabled(enable);
}

void MainWindow::setBootloadBusy(bool busy)
{
    if (busy)
    {
        QApplication::setOverrideCursor(Qt::BusyCursor);
    }
    else
    {
        QApplication::restoreOverrideCursor();
    }

    ui->action_Settings->setEnabled(!busy);
    ui->actionErase_Device->setEnabled(!busy && !bootloader->writeConfig);
    ui->actionWrite_Device->setEnabled(!busy && bootloader->hexOpen);
    ui->actionExit->setEnabled(!busy);
    ui->action_Verify_Device->setEnabled(!busy && bootloader->hexOpen);
    ui->actionOpen->setEnabled(!busy);
    ui->action_Settings->setEnabled(!busy);
    ui->actionBlank_Check->setEnabled(!busy && !bootloader->writeConfig);
    ui->actionReset_Device->setEnabled(!busy);

    comboMemoryRanges.setEnabled(!busy);
}

void MainWindow::updateProgressBar(int newValue) {
    ui->progressBar->setValue(newValue);
}

void MainWindow::onMessage(Bootloader::MessageType type, QString value) {
    if (type == Bootloader::Warning) {
        QMessageBox::warning(this, "Warning!", value, QMessageBox::AcceptRole, QMessageBox::AcceptRole);
        value = "Warning: " + value;
    }
    if (type == Bootloader::Error) {
        QMessageBox::critical(this, "Error!", value, QMessageBox::AcceptRole, QMessageBox::AcceptRole);
        value = "Error: " + value;
    }
    ui->plainTextEdit->appendPlainText(value);
}

void MainWindow::onMessageClear() {
    ui->plainTextEdit->clear();
}


void MainWindow::on_actionExit_triggered() {
    QApplication::exit();
}

void MainWindow::on_action_Verify_Device_triggered()
{
    future = QtConcurrent::run(bootloader, &Bootloader::VerifyDevice);
}

//Gets called when the user clicks to program button in the GUI.
void MainWindow::on_actionWrite_Device_triggered()
{
    future = QtConcurrent::run(bootloader, &Bootloader::WriteDevice);

    onMessageClear();
    onMessage(Bootloader::Info, "Starting Erase/Program/Verify Sequence.");
    onMessage(Bootloader::Info, "Do not unplug device or disconnect power until the operation is fully complete.");
    onMessage(Bootloader::Info, " ");
}

void MainWindow::on_actionBlank_Check_triggered()
{
    future = QtConcurrent::run(bootloader, &Bootloader::BlankCheckDevice);
}

void MainWindow::on_actionErase_Device_triggered()
{
    future = QtConcurrent::run(bootloader, &Bootloader::EraseDevice);
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

void MainWindow::on_action_Settings_triggered()
{
    Comm::ErrorCode result;
    Settings* dlg = new Settings(this);

    dlg->enableEepromBox(bootloader->device->hasEeprom());

    dlg->setWriteFlash(bootloader->writeFlash);
    dlg->setWriteConfig(bootloader->writeConfig);
    dlg->setWriteEeprom(bootloader->writeEeprom);

    if(dlg->exec() == QDialog::Accepted)
    {
        bootloader->writeFlash = dlg->writeFlash;
        bootloader->writeEeprom = dlg->writeEeprom;

        if(!bootloader->writeConfig && dlg->writeConfig)
        {
            ui->plainTextEdit->appendPlainText("Disabling Erase button to prevent accidental erasing of the configuration words without reprogramming them\n");
            bootloader->writeConfig = true;
            bootloader->hexOpen = false;
            result = bootloader->comm->LockUnlockConfig(false);
            if(result == Comm::Success)
            {
                ui->plainTextEdit->appendPlainText("Unlocked Configuration bits successfully\n");
                bootloader->GetQuery();
            }
        }
        else if(bootloader->writeConfig && !dlg->writeConfig)
        {
            bootloader->writeConfig = false;
            bootloader->hexOpen = false;
            result = bootloader->comm->LockUnlockConfig(true);
            if(result == Comm::Success)
            {
                ui->plainTextEdit->appendPlainText("Locked Configuration bits successfully\n");
                bootloader->GetQuery();
            }
        }

        if(!(bootloader->writeFlash || bootloader->writeEeprom || bootloader->writeConfig))
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

void MainWindow::on_actionReset_Device_triggered()
{
    if(!bootloader->comm->isConnected())
    {
        failed = -1;
        qWarning("Reset not sent, device not connected");
        return;
    }

    ui->plainTextEdit->appendPlainText("Resetting...");
    bootloader->comm->Reset();
}


void MainWindow::LoadFile(QString newFileName)
{
    HexImporter::ErrorCode result;

    QApplication::setOverrideCursor(Qt::BusyCursor);

    result = bootloader->LoadFile(newFileName);

    if (result == HexImporter::Success) {
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
    }

    QApplication::restoreOverrideCursor();
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

    for (i = 0; i < recentFileCount; i++)
    {
        text = tr("&%1 %2").arg(i + 1).arg(QFileInfo(files[i]).fileName());

        recentFiles[i]->setText(text);
        recentFiles[i]->setData(files[i]);
        recentFiles[i]->setVisible(bootloader->comm->isConnected());
    }

    for(; i < MAX_RECENT_FILES; i++)
    {
        recentFiles[i]->setVisible(false);
    }
}


void MainWindow::on_actionSaveMemoryRange_triggered()
{

}

void MainWindow::on_actionRead_Device_triggered()
{

}
