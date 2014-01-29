#include "updateavailabledialog.h"
#include "ui_updateavailabledialog.h"
//------------------------------------------------------------------------------------------------------------//
UpdateAvailableDialog::UpdateAvailableDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::UpdateAvailableDialog)
{
    ui->setupUi(this);

    connect(ui->UpdateNowBtn, SIGNAL(clicked()), this, SLOT(StartDownloader()));
}
//------------------------------------------------------------------------------------------------------------//
UpdateAvailableDialog::~UpdateAvailableDialog()
{
    delete ui;
}
//------------------------------------------------------------------------------------------------------------//
void UpdateAvailableDialog::ShowUpdateDialog(QString AppFile, QString ReleaseNotes)
{
    DownloadedFileUrl = AppFile;
    ui->ReleaseNotesTxt->setPlainText(ReleaseNotes);
    this->show();
}
//------------------------------------------------------------------------------------------------------------//
void UpdateAvailableDialog::StartDownloader()
{
    QStringList Arguments;
    Arguments << DownloadedFileUrl;

    QString AppPath = QCoreApplication::applicationDirPath();

    QProcess::startDetached(AppPath + "/viking-upd.exe", Arguments);

    this->close();

    QCoreApplication::exit();
}
//------------------------------------------------------------------------------------------------------------//
