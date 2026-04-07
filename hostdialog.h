#ifndef HOSTDIALOG_H
#define HOSTDIALOG_H

#include <QDialog>
#include "data_processor.h"

namespace Ui {
class HostDialog;
}

class HostDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HostDialog(QWidget *parent = nullptr, bool _isEdit = false);
    ~HostDialog();

    MqttHost getData() const;
    void setData(const MqttHost& host);

private slots:
    void onOkClicked();

    void on_testButton_clicked();

private:
    Ui::HostDialog *ui;
    MqttHost mqtthost;
    bool isEdit;
};

#endif // HOSTDIALOG_H
