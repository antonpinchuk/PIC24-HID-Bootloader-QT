#include "aboutappdialog.h"
#include "ui_aboutappdialog.h"

AboutAppDialog::AboutAppDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutAppDialog)
{
    ui->setupUi(this);
}

AboutAppDialog::~AboutAppDialog()
{
    delete ui;
}
