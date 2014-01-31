#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QApplication>
#include <QFile>
#include <QTextStream>
#include <QByteArray>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QFileInfo>
#include <QProcess>
#include <QDir>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
private:
    Ui::MainWindow *ui;

    QNetworkAccessManager *WebCtrl;
    QByteArray            DownloadedData;

    QString FileUrl;
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void DownloadProgress(qint64 bytesReceived, qint64 bytesTotal );
    void FileDownloaded(QNetworkReply* Reply);
};

#endif // MAINWINDOW_H
