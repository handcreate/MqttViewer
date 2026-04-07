#ifndef SUBSCRIBEDIALOG_H
#define SUBSCRIBEDIALOG_H

#include <QDialog>
#include "data_processor.h"

namespace Ui {
class SubscribeDialog;
}

class SubscribeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SubscribeDialog(QWidget *parent = nullptr, int _hostId = 0, bool _isEdit = false);
    ~SubscribeDialog();

    MqttSubscribe getData() const;
    void setData(const MqttSubscribe& host);

private slots:
    void onOkClicked();

private:
    Ui::SubscribeDialog *ui;
    int hostId;
    MqttSubscribe mqttsubscribe;
    bool isEdit;
};

#endif // SUBSCRIBEDIALOG_H
