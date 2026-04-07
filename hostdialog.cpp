#include "hostdialog.h"
#include "ui_hostdialog.h"
#include "mqtt/async_client.h"
#include <QPushButton>

HostDialog::HostDialog(QWidget *parent, bool _isEdit)
    : QDialog(parent)
    , isEdit(_isEdit)
    , ui(new Ui::HostDialog)
{
    ui->setupUi(this);

    if(isEdit)
    {
        ui->nameEdit->setReadOnly(true);
        ui->nameEdit->setStyleSheet("QLineEdit { background-color: #f0f0f0; }");
    }

    // 断开默认关闭行为，变成自己控制
    QPushButton *okBtn = ui->buttonBox->button(QDialogButtonBox::Ok);
    disconnect(ui->buttonBox, &QDialogButtonBox::accepted,
               this, &QDialog::accept);
    connect(okBtn, &QPushButton::clicked, this, &HostDialog::onOkClicked);
}

HostDialog::~HostDialog()
{
    delete ui;
}

MqttHost HostDialog::getData() const
{
    return mqtthost;
}

void HostDialog::setData(const MqttHost& host)
{
    ui->nameEdit->setText(host.name);
    ui->addressEdit->setText(host.address);
    ui->usernameEdit->setText(host.username);
    ui->passwordEdit->setText(host.password);
}

void HostDialog::onOkClicked()
{
    QString name = ui->nameEdit->text();
    if(!isEdit)
    {
        if(name.trimmed().isEmpty())
        {
            ui->hintLabel->setText("<p style='color:red'>连接名称不能为空</p>");
            return;
        }

        if(findHostConfig(name))
        {
            ui->hintLabel->setText("<p style='color:red'>连接名称已存在</p>");
            return;
        }
    }

    QString address = ui->addressEdit->text();
    if(address.trimmed().isEmpty())
    {
        ui->hintLabel->setText("<p style='color:red'>地址不能为空</p>");
        return;
    }
    QString username = ui->usernameEdit->text();
    if(username.isEmpty())
    {
        ui->hintLabel->setText("<p style='color:red'>用户名不能为空</p>");
        return;
    }
    QString password = ui->passwordEdit->text();
    if(password.isEmpty())
    {
        ui->hintLabel->setText("<p style='color:red'>密码不能为空</p>");
        return;
    }

    mqtthost.name = name;
    mqtthost.address = address;
    mqtthost.username = username;
    mqtthost.password = password;

    accept();
}


void HostDialog::on_testButton_clicked()
{
    QString address = ui->addressEdit->text();
    if(address.trimmed().isEmpty())
    {
        ui->hintLabel->setText("<p style='color:red'>地址不能为空</p>");
        return;
    }
    QString username = ui->usernameEdit->text();
    if(username.isEmpty())
    {
        ui->hintLabel->setText("<p style='color:red'>用户名不能为空</p>");
        return;
    }
    QString password = ui->passwordEdit->text();
    if(password.isEmpty())
    {
        ui->hintLabel->setText("<p style='color:red'>密码不能为空</p>");
        return;
    }

    try
    {
        mqtt::async_client client(address.toStdString(), "conn_test");

        mqtt::connect_options connOpts = mqtt::connect_options_builder()
            .clean_session(true)
            .automatic_reconnect(true)
            .user_name(username.toStdString())
            .password(password.toStdString())
            .finalize();

        // 尝试连接
        client.connect(connOpts)->wait();

        // 成功就断开
        client.disconnect()->wait();

        ui->hintLabel->setText("<p style='color:green'>连接成功</p>");
    }
    catch (const mqtt::exception& e)
    {
        ui->hintLabel->setText(QString("<p style='color:red'>连接失败：%1</p>").arg(e.what()));
    }
}

