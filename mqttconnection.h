#ifndef MQTTCONNECTION_H
#define MQTTCONNECTION_H

#include "data_processor.h"
#include "mqtt/async_client.h"
#include <QStandardItemModel>

class MqttConnection
{  
    class MQTTCallback : public virtual mqtt::callback
    {
    public:
        void message_arrived(mqtt::const_message_ptr msg) override;
        // void delivery_complete(mqtt::delivery_token_ptr token) override {}
        // void connected(const std::string& cause) override {}
        // void connection_lost(const std::string& cause) override {}
    public:
        MqttHost* m_pHost;
    };

public:
    MqttConnection(int _host_id);
    ~MqttConnection();

    bool open();
    bool close();
    void subscribe(QString topic, int qos = 0);
    void unsubscribe(QString topic);
    bool publish(QString topic, QString payload, int qos = 0, bool retained = false);

private:
    std::shared_ptr<mqtt::async_client> client;
    mqtt::connect_options conn_opts;
    MQTTCallback callback;
    int host_id;
    MqttHost* m_pHost;
    QStandardItemModel *m_listModel;
    int max_buffer_size;
    int keep_alive_interval;
public:
    bool isConnected;
};

#endif // MQTTCONNECTION_H
