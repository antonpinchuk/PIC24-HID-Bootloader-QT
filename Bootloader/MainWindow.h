/************************************************************************
* Copyright (c) 2009-2010,  Microchip Technology Inc.
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
*/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QLabel>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QtCore/QProcess>
#include <QtWidgets/QMenu>
#include <QComboBox>
#include <QFuture>

#include "HIDBootloader/Bootloader.h"
//#include "HIDBootloader/Comm.h"
//#include "HIDBootloader/DeviceData.h"
//#include "HIDBootloader/Device.h"


namespace Ui
{
    class MainWindowClass;
}

#define MAX_RECENT_FILES 3

/*!
 * The main Serial Bootloader GUI window.
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    Bootloader *bootloader;

    MainWindow(QWidget *parent = 0);
    ~MainWindow();

signals:
    void AppendString(QString msg);

public slots:
    void openRecentFile(void);

protected:
    QFuture<void> future;

    QString fileName, watchFileName;
    QFileSystemWatcher* fileWatcher;

    void UpdateRecentFileList(void);

private:
    Ui::MainWindowClass *ui;
    QLabel deviceLabel;

    int failed;
    QAction *recentFiles[MAX_RECENT_FILES];

    bool wasBootloaderMode;

    void LoadFile(QString fileName);

    QLabel labelMemoryRanges;
    QComboBox comboMemoryRanges;

private slots:
    void setConnected(bool enable);
    void setBootloadEnabled(bool enable);
    void setBootloadBusy(bool busy);
    void updateProgressBar(int newValue);
    void onMessage(Bootloader::MessageType type, QString value);
    void onMessageClear();

    void onChangedMemoryRanges(int value);


    void on_actionBlank_Check_triggered();
    void on_actionReset_Device_triggered();
    void on_action_Settings_triggered();
    void on_action_Verify_Device_triggered();
    void on_action_About_triggered();
    void on_actionWrite_Device_triggered();
    void on_actionOpen_triggered();
    void on_actionErase_Device_triggered();
    void on_actionExit_triggered();
    void on_actionSaveMemoryRange_triggered();
    void on_actionRead_Device_triggered();
};

#endif // MAINWINDOW_H
