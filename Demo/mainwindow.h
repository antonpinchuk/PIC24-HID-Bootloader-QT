#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <QtWidgets/QLabel>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QtConcurrent/QtConcurrentRun>

#include "HIDBootloader/Bootloader.h"
#include "aboutappdialog.h"

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow   *ui;

    Bootloader  *bootloader;

    QString fileName;

    AboutAppDialog        *aboutAppDialog;

    QFuture<void> future;

    void writeDevice(unsigned char range);
    void readDevice(unsigned char range);

private slots:

    /* Bootloader event */
    void setConnected(bool enable);
    void setBootloadEnabled(bool enable);
    void setBootloadBusy(bool busy);
    void updateProgressBar(int newValue);
    void onMessage(Bootloader::MessageType type, QString value);
    void onMessageClear();
    void onReadComplete();
    void onWriteComplete();

    void on_WriteDeviceBtn_clicked();
    void on_ReadDeviceBtn_clicked();

    void on_AboutButton_clicked();
};

#endif // MAINWINDOW_H
