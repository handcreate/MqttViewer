#include "mqttconnection.h"
#include "messagelist.h"
#include <QDebug>
#include <QDateTime>

void MqttConnection::MQTTCallback::message_arrived(mqtt::const_message_ptr msg)
{
    QString text  = QString::fromUtf8(msg->to_string().c_str());
    QString topic = QString::fromUtf8(msg->get_topic().c_str());
    m_pHost->msglist->insert(topic, text, false);
}

MqttConnection::MqttConnection(int _host_id)
    : host_id(_host_id)
    , isConnected(false)
    , max_buffer_size(100)
    , keep_alive_interval(10)
{
    m_pHost = &g_mqttconfig.hosts[host_id];
}

MqttConnection::~MqttConnection()
{
    if(isConnected)
    {
        close();
    }
}

bool MqttConnection::open()
{
    if(isConnected)
        return true;

    if(!m_pHost)
    {
        qCritical() << "未初始化";
        return false;
    }

    try
    {
        client = std::make_shared<mqtt::async_client>(
            m_pHost->address.toStdString(),
            m_pHost->name.toStdString(),
            max_buffer_size);
        conn_opts = mqtt::connect_options_builder()
            .keep_alive_interval(std::chrono::seconds(keep_alive_interval))
            .clean_session(true)
            .automatic_reconnect(true)
            .user_name(m_pHost->username.toStdString())
            .password(m_pHost->password.toStdString())
            .finalize();

        callback.m_pHost = m_pHost;
        client->set_callback(callback);
        client->connect(conn_opts)->wait();

        for(auto &s : m_pHost->subscribes)
        {
            if(s.checked)
            {
                client->subscribe(s.topic.toStdString(), s.qos);
            }
        }

        isConnected = true;
        qInfo() << "连接 [" << m_pHost->address << "] 建立成功";
        return true;
    }
    catch(const mqtt::exception& exc)
    {
        isConnected = false;
        qCritical() << "连接失败：" << exc.what();
        return false;
    }
}

bool MqttConnection::close()
{
    if(isConnected)
    {
        client->disconnect()->wait();
        client.reset();
        isConnected = false;

        qInfo() << "连接 [" << m_pHost->address << "] 已关闭";
    }
    return true;
}

void MqttConnection::subscribe(QString topic, int qos)
{
    try {
        client->subscribe(topic.toStdString(), qos);
    } catch (...) {
    }
}

void MqttConnection::unsubscribe(QString topic)
{
    client->unsubscribe(topic.toStdString())->wait();
}

bool MqttConnection::publish(QString topic,
    QString payload, int qos, bool retained)
{
    try {
        client->publish(topic.toStdString(), payload.toStdString(), 0, retained);
        return true;
    } catch (...) {
        return false;
    }
}