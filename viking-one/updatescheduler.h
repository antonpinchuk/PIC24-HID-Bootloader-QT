#ifndef UPDATESCHEDULER_H
#define UPDATESCHEDULER_H
//------------------------------------------------------------------------------------------------------------//
#include <QObject>
#include <QTimer>
#include <QTime>

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

#include <QJsonDocument>
#include <QJsonObject>

#include <QFile>
#include <QTextStream>

#include "updateavailabledialog.h"
//------------------------------------------------------------------------------------------------------------//
class TUpdateScheduler : public QObject
{
    Q_OBJECT

private:
    QString UpdateJsonUrl; // адрес сайта, откуда брать файл с инфой об обновлении

    QNetworkAccessManager* WebCtrl;

    QTimer *UpdateRequestTimer;
    int     StartAppUpdateTime;
    QTime   SchedulerTime;
public:
    explicit TUpdateScheduler(QObject *parent = 0);
    virtual ~TUpdateScheduler();
protected slots:
    void CheckUpdateTime();
    void CheckCurrVersion(const QByteArray &DownloadedData);
    void JsonFileDownloaded(QNetworkReply* Reply);

public slots:
    void CheckUpdate();

signals:
    void DownloadReady();
    void NetworkErr();
    void NewVersionAvailable(QString VersionNotise);
    void NeedUpdate(QString AppFile, QString ReleaseNotes);
};
//------------------------------------------------------------------------------------------------------------//
#endif // UPDATESCHEDULER_H
