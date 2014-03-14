#include "mainwindow.h"
#include "ui_mainwindow.h"

#define MEMORY_RANGE_PROGRAM 0
#define MEMORY_RANGE_EEPROM 1


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setFixedSize(this->minimumWidth(), this->minimumHeight());

    QString VersionStr(APPLICATION_VERSION);
    this->setWindowTitle("PIC24 HID Bootloader - Integration Demo " + VersionStr);

    aboutAppDialog = new AboutAppDialog(this);

    /* Bootloader */
    bootloader = new Bootloader();
    bootloader->writeFlash       = true;
    bootloader->writeEeprom      = true;
    bootloader->writeConfig      = false; //Force user to manually re-enable it every time they re-launch the application.  Safer that way.
    bootloader->eraseDuringWrite = true;

    connect(bootloader, SIGNAL(setConnected(bool)), this, SLOT(setConnected(bool)));
    connect(bootloader, SIGNAL(setBootloadEnabled(bool)), this, SLOT(setBootloadEnabled(bool)));
    connect(bootloader, SIGNAL(setBootloadBusy(bool)), this, SLOT(setBootloadBusy(bool)));
    connect(bootloader, SIGNAL(setProgressBar(int)), this, SLOT(updateProgressBar(int)));
    connect(bootloader, SIGNAL(message(Bootloader::MessageType, QString)), this, SLOT(onMessage(Bootloader::MessageType, QString)));
    connect(bootloader, SIGNAL(messageClear()), this, SLOT(onMessageClear()));
    connect(bootloader, SIGNAL(ReadComplete()), this, SLOT(onReadComplete()));
    connect(bootloader, SIGNAL(WriteComplete()), this, SLOT(onWriteComplete()));

    setConnected(false);
    setBootloadEnabled(false);
}

MainWindow::~MainWindow() {
    delete ui;
    delete aboutAppDialog;
}


void MainWindow::setConnected(bool connected) {
    if (connected) {
        ui->StatusLbl->setText("Status: Connected");

        QPalette palette = ui->StatusLbl->palette();
        palette.setColor(ui->StatusLbl->backgroundRole(), Qt::darkGreen);
        palette.setColor(ui->StatusLbl->foregroundRole(), Qt::darkGreen);
        ui->StatusLbl->setPalette(palette);

        setBootloadEnabled(true);
    } else {
        ui->StatusLbl->setText("Status: Disconnected");

        QPalette palette = ui->StatusLbl->palette();
        palette.setColor(ui->StatusLbl->backgroundRole(), Qt::red);
        palette.setColor(ui->StatusLbl->foregroundRole(), Qt::red);
        ui->StatusLbl->setPalette(palette);

        setBootloadEnabled(false);
    }
}

void MainWindow::setBootloadEnabled(bool enable) {
    ui->WriteDeviceBtn->setEnabled(enable);
    ui->ReadDeviceBtn->setEnabled(enable);
}

void MainWindow::setBootloadBusy(bool busy) {
    if (busy) {
        QApplication::setOverrideCursor(Qt::BusyCursor);
    } else {
        QApplication::restoreOverrideCursor();
    }
    setBootloadEnabled(!busy);
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

void MainWindow::onReadComplete() {
    // Save file uppon read completion
    bootloader->SaveFile(fileName);
}

void MainWindow::onWriteComplete() {

}

void MainWindow::updateProgressBar(int newValue) {
    ui->progressBar->setValue(newValue);
}


void MainWindow::on_WriteDeviceBtn_clicked() {
    writeDevice(MEMORY_RANGE_PROGRAM);
}

void MainWindow::writeDevice(unsigned char range) {
    HexImporter::ErrorCode result;
    onMessageClear();

    fileName = QFileDialog::getOpenFileName(this, "Open Hex File", fileName, "Hex Files (*.hex *.ehx)");
    if (fileName.isEmpty()) {
        return;
    }

    QApplication::setOverrideCursor(Qt::BusyCursor);
    result = bootloader->LoadFile(fileName);
    QApplication::restoreOverrideCursor();

    if (result == HexImporter::Success) {
        onMessage(Bootloader::Info, "Starting Erase/Program/Verify Sequence.");
        onMessage(Bootloader::Info, "Do not unplug device or disconnect power until the operation is fully complete.");
        onMessage(Bootloader::Info, " ");

        bootloader->rangeReadWrite = range;

        future = QtConcurrent::run(bootloader, &Bootloader::WriteDevice);
    }
}

void MainWindow::on_ReadDeviceBtn_clicked() {
    readDevice(MEMORY_RANGE_PROGRAM);
}

void MainWindow::readDevice(unsigned char range) {
    fileName =
        QFileDialog::getSaveFileName(this, "Save Binary File", "DeviceProgramMemoryDump.bin", "Binary Files (*.bin)");
    if (fileName.isEmpty()) {
        return;
    }

    bootloader->rangeReadWrite = range;
    future = QtConcurrent::run(bootloader, &Bootloader::ReadDevice);
}

void MainWindow::on_AboutButton_clicked() {
    aboutAppDialog->open();
}

