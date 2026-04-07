#include "subscribedialog.h"
#include "ui_subscribedialog.h"
#include "common_utils.h"
#include <QPushButton>

SubscribeDialog::SubscribeDialog(QWidget *parent, int _hostId, bool _isEdit)
    : QDialog(parent)
    , hostId(_hostId)
    , isEdit(_isEdit)
    , ui(new Ui::SubscribeDialog)
{
    ui->setupUi(this);
    ui->colorWidget->setFixedSize(100, 25);
    if(!isEdit)
    {
        QColor color = randomColor();
        ui->colorWidget->setColor(color);
    }
    else
    {
        ui->nameEdit->setReadOnly(true);
        ui->nameEdit->setStyleSheet("QLineEdit { background-color: #f0f0f0; }");
    }

    // 断开默认关闭行为，变成自己控制
    QPushButton *okBtn = ui->buttonBox->button(QDialogButtonBox::Ok);
    disconnect(ui->buttonBox, &QDialogButtonBox::accepted,
               this, &QDialog::accept);
    connect(okBtn, &QPushButton::clicked, this, &SubscribeDialog::onOkClicked);
}

SubscribeDialog::~SubscribeDialog()
{
    delete ui;
}

MqttSubscribe SubscribeDialog::getData() const
{
    return mqttsubscribe;
}

void SubscribeDialog::setData(const MqttSubscribe& subs)
{
    ui->nameEdit->setText(subs.name);
    ui->topicEdit->setText(subs.topic);
    ui->qosBox->setCurrentIndex(subs.qos);
    ui->colorWidget->setColor(QColor(subs.color));
    ui->remindCheckBox->setChecked(subs.remind);
}

void SubscribeDialog::onOkClicked()
{
    QString name = ui->nameEdit->text();
    if(!isEdit)
    {
        if(name.trimmed().isEmpty())
        {
            ui->hintLabel->setText("<p style='color:red'>订阅名称不能为空</p>");
            return;
        }

        if(findSubscribeConfig(hostId, name))
        {
            ui->hintLabel->setText("<p style='color:red'>订阅名称已存在</p>");
            return;
        }
    }

    QString topic = ui->topicEdit->text();
    if(topic.trimmed().isEmpty())
    {
        ui->hintLabel->setText("<p style='color:red'>主题不能为空</p>");
        return;
    }

    int qos = ui->qosBox->currentIndex();

    QColor color = ui->colorWidget->color();

    bool remind = ui->remindCheckBox->isChecked();

    mqttsubscribe.name = name;
    mqttsubscribe.topic = topic;
    mqttsubscribe.qos = qos;
    mqttsubscribe.color = color.name();
    mqttsubscribe.remind = remind;

    accept();
}
