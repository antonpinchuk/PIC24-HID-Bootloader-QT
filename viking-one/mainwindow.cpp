#include "mainwindow.h"
#include "ui_mainwindow.h"
//------------------------------------------------------------------------------------------------------------//
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    updateAvailableDialog = new UpdateAvailableDialog(this);
    UpdateScheduler       = new TUpdateScheduler(this);
    SystemTrayMenu        = new QMenu("tray menu");

    QAction* checkUpdateAction = SystemTrayMenu->addAction("check update");
    QAction* quitAction        = SystemTrayMenu->addAction("quit");

    connect(quitAction       , SIGNAL(triggered()), this, SLOT(Exit()) );
    connect(checkUpdateAction, SIGNAL(triggered()), UpdateScheduler, SLOT(CheckUpdate()) );

    Ico = new QSystemTrayIcon(this);
    Ico->setIcon(QIcon(":/new/ico/app.ico"));
    Ico->setContextMenu(SystemTrayMenu);
    Ico->show();

    connect(Ico , SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
            this, SLOT  (TrayIcoClick(QSystemTrayIcon::ActivationReason)) );



    connect( ui->CheckVersionBtn , SIGNAL(clicked()), UpdateScheduler, SLOT(CheckUpdate()) );
    connect( UpdateScheduler, SIGNAL(NeedUpdate(QString,QString)), this, SLOT(NeedUpdate(QString,QString)) );

    ui->CheckVersionBtn->setVisible(false);
}
//------------------------------------------------------------------------------------------------------------//
MainWindow::~MainWindow()
{
    delete ui;
    delete Ico;
    delete UpdateScheduler;
    delete updateAvailableDialog;
    delete SystemTrayMenu;
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
        if(!this->isHidden()){
            this->hide();
        }
        else{
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
void MainWindow::Exit()
{
    QCoreApplication::exit();
}
//------------------------------------------------------------------------------------------------------------//

void MainWindow::on_WriteRunePackBtn_clicked()
{
    QString msg, newFileName;
    QTextStream stream(&msg);

    newFileName = QFileDialog::getOpenFileName(this, "Open Hex File", fileName, "Hex Files (*.hex *.ehx)");

    if (newFileName.isEmpty()) {
        return;
    }

    //LoadFile(newFileName);
}
