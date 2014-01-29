#include "mainwindow.h"
#include "ui_mainwindow.h"
//------------------------------------------------------------------------------------------------------------//
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    updateAvailableDialog = new UpdateAvailableDialog(this);

    Ico = new QSystemTrayIcon(this);
    Ico->setIcon(QIcon(":/new/ico/app.ico"));
    Ico->show();

    UpdateScheduler = new TUpdateScheduler(this);

    connect(Ico , SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
            this, SLOT  (TrayIcoClick(QSystemTrayIcon::ActivationReason)) );


    connect( ui->CheckVersionBtn , SIGNAL(clicked()), UpdateScheduler, SLOT(CheckUpdate()) );
    connect( UpdateScheduler, SIGNAL(NeedUpdate(QString,QString)), this, SLOT(NeedUpdate(QString,QString)) );

    // ui->CheckVersionBtn->setVisible(false);
}
//------------------------------------------------------------------------------------------------------------//
MainWindow::~MainWindow()
{
    delete ui;
    delete Ico;
    delete UpdateScheduler;
    delete updateAvailableDialog;
}
//------------------------------------------------------------------------------------------------------------//
void MainWindow::changeEvent( QEvent * event )
{
    // упрятывание приложения в трей
    if(event->type() == QEvent::WindowStateChange) {
        if(isMinimized()) {
            this->hide();
            event->ignore();
        }
    }
    QMainWindow::changeEvent(event);
}
//------------------------------------------------------------------------------------------------------------//
void MainWindow::TrayIcoClick(QSystemTrayIcon::ActivationReason Reason)
{
    if(Reason == QSystemTrayIcon::Trigger){
        if(this->isHidden()){
            this->showNormal();
            this->activateWindow();
        }
    }
}
//------------------------------------------------------------------------------------------------------------//
void MainWindow::NeedUpdate(QString AppFile, QString ReleaseNotes)
{
    updateAvailableDialog->ShowUpdateDialog(AppFile, ReleaseNotes);
}
//------------------------------------------------------------------------------------------------------------//
