#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    WebCtrl = new QNetworkAccessManager(this);

    if(QApplication::arguments().size() > 1){
        // FileUrl = "http://www.viking360.com/downloads/xbox-one/berserker/updates/test.1.0.0.1401211601.png";
        FileUrl = QApplication::arguments().at(1);

        QNetworkRequest Request(FileUrl);
        QNetworkReply* CurrDownload = WebCtrl->get(Request);
        connect(CurrDownload, SIGNAL(downloadProgress(qint64 ,qint64)), this, SLOT(DownloadProgress(qint64, qint64)));
        connect(WebCtrl     , SIGNAL(finished(QNetworkReply*))        , this, SLOT(FileDownloaded(QNetworkReply*)));
    }
}

MainWindow::~MainWindow()
{
    delete ui;
    delete WebCtrl;
}

void MainWindow::DownloadProgress(qint64 bytesReceived, qint64 bytesTotal )
{
    float DownloadedVal = (float)bytesTotal/(float)bytesReceived;

    ui->DownloadProgressBar->setValue(DownloadedVal*100);
}

void MainWindow::FileDownloaded(QNetworkReply* Reply)
{
    if(Reply->error() != QNetworkReply::NoError){
        Reply->deleteLater();
        return;
    }

    DownloadedData = Reply->readAll();
    Reply->deleteLater();

    QFileInfo FileInf(FileUrl);
    QString FileName = FileInf.fileName();

    QFile File("updates/" + FileName);
    File.open(QIODevice::WriteOnly);
    File.write(DownloadedData);
    File.close();

    QString AppPath = QCoreApplication::applicationDirPath();
    QProcess::startDetached(AppPath + "updates/" + FileName);

    this->close();
    QCoreApplication::exit();
}

