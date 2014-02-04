#ifndef ABOUTAPPDIALOG_H
#define ABOUTAPPDIALOG_H

#include <QDialog>

namespace Ui {
class AboutAppDialog;
}

class AboutAppDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AboutAppDialog(QWidget *parent = 0);
    ~AboutAppDialog();

private:
    Ui::AboutAppDialog *ui;
};

#endif // ABOUTAPPDIALOG_H
