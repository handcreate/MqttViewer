#ifndef DATA_PROCESSOR_H
#define DATA_PROCESSOR_H

#include <QString>
#include <QVector>
#include <QMap>

class MqttConnection;
class MessageList;

// 单个订阅
struct MqttSubscribe
{
    QString name;
    QString topic;
    int qos = 0;
    QString color;
    bool remind = false;
    bool checked = false;
    uint msg_count = 0;
};

// 单个 MQTT 主机
struct MqttHost
{
    QString name;
    QString address;
    QString username;
    QString password;
    QVector<MqttSubscribe> subscribes;
    QVector<QString> recentTopics;
    QString recentSendMessage;
    std::shared_ptr<MqttConnection> connection;
    MessageList *msglist;

    QMap<QString, int> topic_subid_map; // 完整topic与订阅ID的映射
};

// 总配置
struct MqttConfig
{
    QVector<MqttHost> hosts;
    int host_id = -1; // 当前浏览host

    void clear()
    {
        hosts.clear();
    }
};

extern MqttConfig g_mqttconfig;
QString getConfigFilePath();
QString getCachePath();
void loadConfig();
bool saveConfig();
MqttHost* findHostConfig(const QString& name);
int addHostConfig(const MqttHost& host);
void editHostConfig(int host_id, const MqttHost& host);
void deleteHostConfig(int host_id);
MqttSubscribe* findSubscribeConfig(int host_id, const QString& name);
int findSubscribeByFullTopic(int host_id, const QString& fullTopic);
void addSubscribeConfig(int host_id, const MqttSubscribe& subs);
void editSubscribeConfig(int host_id, int subs_id, const MqttSubscribe& subs);
void deleteSubscribeConfig(int host_id, int subs_id);
void addRecentTopics(int host_id, const QString& topic, const QString& msg);
void editSubscribeChecked(int host_id, int subs_id, bool checked);
void resetMessageCount(int host_id);

#endif // DATA_PROCESSOR_H
