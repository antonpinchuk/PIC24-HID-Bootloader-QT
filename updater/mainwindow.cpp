#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QString VersionStr(APPLICATION_VERSION);
    this->setWindowTitle("Viking update downloader - Version " + VersionStr);

    WebCtrl = new QNetworkAccessManager(this);

    // default path to updater
    FileUrl = "http://www.viking360.com/downloads/xbox-one/berserker/file";
    if(QApplication::arguments().size() > 1){
        FileUrl = QApplication::arguments().at(1);
    }

    QNetworkRequest Request(FileUrl);
    QNetworkReply* CurrDownload = WebCtrl->get(Request);
    connect(CurrDownload, SIGNAL(downloadProgress(qint64 ,qint64)), this, SLOT(DownloadProgress(qint64, qint64)));
    connect(WebCtrl     , SIGNAL(finished(QNetworkReply*))        , this, SLOT(FileDownloaded(QNetworkReply*)));
}

MainWindow::~MainWindow()
{
    delete ui;
    delete WebCtrl;
}

void MainWindow::DownloadProgress(qint64 bytesReceived, qint64 bytesTotal )
{
    ui->DownloadProgressBar->setValue( (100.0/bytesTotal)*(double)bytesReceived );
}

void MainWindow::FileDownloaded(QNetworkReply* Reply)
{
    if(Reply->error() != QNetworkReply::NoError){
        Reply->deleteLater();
        return;
    }

    DownloadedData = Reply->readAll();
    Reply->deleteLater();


    // QFileInfo FileInf(FileUrl);
    // QString FileName = FileInf.fileName();

    QString FileName = "viking-one-setup.exe";

    // add update catalog to user folder
    QDir Dir;
    QString QUserUpdatesDir = Dir.homePath() + "/viking-one";
    if(!Dir.exists(QUserUpdatesDir)){ Dir.mkdir(QUserUpdatesDir); }

    QUserUpdatesDir += "/updates";
    if(!Dir.exists(QUserUpdatesDir)){ Dir.mkdir(QUserUpdatesDir); }

    QUserUpdatesDir += "/";


    QFile File(QUserUpdatesDir + FileName );
    File.open(QIODevice::WriteOnly);
    File.write(DownloadedData);
    File.close();

    QStringList Arguments;
    Arguments << "/VERYSILENT";
    Arguments << "/CLOSEAPPLICATIONS";

    QProcess::startDetached(QUserUpdatesDir + FileName, Arguments);

    this->close();
    QCoreApplication::exit();
}

