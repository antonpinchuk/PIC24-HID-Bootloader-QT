#ifndef UPDATEAVAILABLEDIALOG_H
#define UPDATEAVAILABLEDIALOG_H
//------------------------------------------------------------------------------------------------------------//
#include <QDialog>
#include <QProcess>
#include <QCoreApplication>
//------------------------------------------------------------------------------------------------------------//
namespace Ui {
class UpdateAvailableDialog;
}
//------------------------------------------------------------------------------------------------------------//
class UpdateAvailableDialog : public QDialog
{
    Q_OBJECT
private:
    Ui::UpdateAvailableDialog *ui;
    QString DownloadedFileUrl;
public:
    explicit UpdateAvailableDialog(QWidget *parent = 0);
    ~UpdateAvailableDialog();

    void ShowUpdateDialog(QString AppFile, QString ReleaseNotes);

private slots:
    void StartDownloader();
};
//------------------------------------------------------------------------------------------------------------//
#endif // UPDATEAVAILABLEDIALOG_H
