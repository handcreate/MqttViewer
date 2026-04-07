#include "data_processor.h"
#include "mqttconnection.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QApplication>

MqttConfig g_mqttconfig;

bool matchTopic(const QString &sub, const QString &topic)
{
    QStringList subLevels   = sub.split('/', Qt::SkipEmptyParts);
    QStringList topicLevels = topic.split('/', Qt::SkipEmptyParts);

    int i = 0;
    for (; i < subLevels.size(); ++i)
    {
        // topic 不够长
        if (i >= topicLevels.size())
            return false;

        const QString &s = subLevels[i];
        const QString &t = topicLevels[i];

        if (s == "#")
        {
            // # 必须是最后一个
            return true;
        }
        else if (s == "+")
        {
            // 匹配任意一层
            continue;
        }
        else
        {
            if (s != t)
                return false;
        }
    }

    // 如果 topic 还有剩余层，则不匹配（除非最后是 #）
    return i == topicLevels.size();
}

QString getConfigFilePath()
{
    QString fileName = "mqttview.json";

    QString appDir = QCoreApplication::applicationDirPath();
    QDir dir(appDir);

    // 只要 data 目录存在，直接返回（不判断文件是否存在）
    if (dir.exists("data"))
    {
        return QDir::cleanPath(dir.filePath("data/" + fileName));
    }

    // 用户配置目录
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(configDir);

    QString userPath = QDir(configDir).filePath(fileName);

    // 如果存在就返回
    if (QFile::exists(userPath))
        return userPath;

    // 不存在也返回（用于首次创建）
    return userPath;
}

QString getCachePath()
{
    QString confPath = getConfigFilePath();
    // 1. 获取文件所在目录
    QFileInfo fileInfo(confPath);
    QString fileDir = fileInfo.absolutePath(); // 或使用 .path()

    // 2. 构建 cache 子目录的完整路径
    QString cacheDirPath = fileDir + "/cache";

    // 3. 检查目录是否存在，不存在则创建
    QDir cacheDir(cacheDirPath);
    if (!cacheDir.exists()) {
        if (cacheDir.mkpath(".")) { // mkpath 会自动创建所有缺失的父目录
            qDebug() << "创建目录成功:" << cacheDirPath;
        } else {
            qDebug() << "创建目录失败:" << cacheDirPath;
            return QString(); // 返回空字符串表示失败
        }
    }

    return cacheDirPath;
}

bool saveConfig()
{
    QString path = getConfigFilePath();
    QJsonObject root;

    // hosts
    QJsonArray hostsArray;
    for (const auto& h : g_mqttconfig.hosts)
    {
        QJsonObject jhost;
        jhost["name"] = h.name;
        jhost["address"] = h.address;
        jhost["username"] = h.username;
        jhost["password"] = h.password;

        // subscribes
        QJsonArray subsArray;
        for (const auto& s : h.subscribes)
        {
            QJsonObject jsub;
            jsub["name"] = s.name;
            jsub["topic"] = s.topic;
            jsub["qos"] = s.qos;
            jsub["color"] = s.color;
            jsub["remind"] = s.remind;
            jsub["checked"] = s.checked;
            subsArray.append(jsub);
        }
        jhost["subscribes"] = subsArray;

        QJsonArray rtArray;
        for(const auto& t : h.recentTopics)
        {
            rtArray.append(t);
        }
        jhost["recent_topics"] = rtArray;
        jhost["recent_send_msg"] = h.recentSendMessage;

        hostsArray.append(jhost);
    }
    root["hosts"] = hostsArray;


    QJsonDocument doc(root);

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return false;

    file.write(doc.toJson(QJsonDocument::Indented));
    return true;
}

void loadConfig()
{
    g_mqttconfig.clear();

    QString path = getConfigFilePath();

    QFile file(path);

    if (!file.exists())
    {
        qDebug() << "Config not found" << path;
        return;
    }

    if (!file.open(QIODevice::ReadOnly))
    {
        qWarning() << "Failed to open config:" << path;
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject())
    {
        qWarning() << "Invalid JSON, use default";
        return;
    }

    QJsonObject root = doc.object();

    // hosts
    for (auto vh : root["hosts"].toArray())
    {
        QJsonObject jHost = vh.toObject();

        MqttHost h;
        h.name = jHost["name"].toString();
        h.address = jHost["address"].toString();
        h.username = jHost["username"].toString();
        h.password = jHost["password"].toString();

        // subscribes
        for (auto vs : jHost["subscribes"].toArray())
        {
            QJsonObject jSub = vs.toObject();

            MqttSubscribe s;
            s.name = jSub["name"].toString();
            s.topic = jSub["topic"].toString();
            s.qos = jSub["qos"].toInt(0);
            s.color = jSub["color"].toString("#ffffff");
            s.remind = jSub["remind"].toBool();
            s.checked = jSub["checked"].toBool();

            h.subscribes.append(s);
        }

        for (auto rt : jHost["recent_topics"].toArray())
        {
            h.recentTopics.append(rt.toString());
        }
        h.recentSendMessage = jHost["recent_send_msg"].toString();

        g_mqttconfig.hosts.append(h);
    }

}

MqttHost* findHostConfig(const QString& name)
{
    for (auto& h : g_mqttconfig.hosts)
    {
        if (h.name == name)
            return &h;
    }
    return nullptr;
}

int addHostConfig(const MqttHost& host)
{
    g_mqttconfig.hosts.append(host);
    saveConfig();
    return g_mqttconfig.hosts.size() - 1;
}

void editHostConfig(int host_id, const MqttHost& host)
{
    auto &h = g_mqttconfig.hosts[host_id];
    h.address = host.address;
    h.username = host.username;
    h.password = host.password;
    saveConfig();
}

void deleteHostConfig(int host_id)
{
    g_mqttconfig.hosts.remove(host_id);
    saveConfig();
}

MqttSubscribe* findSubscribeConfig(int host_id, const QString& name)
{
    for (auto& s : g_mqttconfig.hosts[host_id].subscribes)
    {
        if (s.name == name)
            return &s;
    }
    return nullptr;
}

void addSubscribeConfig(int host_id, const MqttSubscribe& subs)
{
    g_mqttconfig.hosts[host_id].subscribes.append(subs);
    saveConfig();
}

void editSubscribeConfig(int host_id, int subs_id, const MqttSubscribe& subs)
{
    auto &s = g_mqttconfig.hosts[host_id].subscribes[subs_id];
    s.topic = subs.topic;
    s.qos = subs.qos;
    s.color = subs.color;
    s.remind = subs.remind;
    saveConfig();
}

void deleteSubscribeConfig(int host_id, int subs_id)
{
    // 取消订阅
    std::shared_ptr<MqttConnection> conn = g_mqttconfig.hosts[host_id].connection;
    if(conn && conn->isConnected)
    {
        MqttSubscribe &sub = g_mqttconfig.hosts[host_id].subscribes[subs_id];
        conn->unsubscribe(sub.topic);
    }

    // 删除map映射与本subs_id相关的记录
    QMap<QString, int> *map = &g_mqttconfig.hosts[host_id].topic_subid_map;
    for (auto it = map->begin(); it != map->end(); )
    {
        if (it.value() == subs_id)
            it = map->erase(it);  // 删除并返回下一个迭代器
        else
            ++it;
    }

    // 删除元素
    g_mqttconfig.hosts[host_id].subscribes.remove(subs_id);
    saveConfig();
}

void addRecentTopics(int host_id, const QString& topic, const QString& msg)
{
    if(topic.contains('+') || topic.contains('#'))
        return;

    g_mqttconfig.hosts[host_id].recentSendMessage = msg;

    // 列表里存在的情况
    for(int i=0; i<g_mqttconfig.hosts[host_id].recentTopics.size(); i++)
    {
        auto &t = g_mqttconfig.hosts[host_id].recentTopics[i];
        if(t == topic)
        {
            if(i == 0)
            {
                saveConfig();
                return; // 和队列的第一个相同，不处理
            }
            else
            {
                // 移动到队列的第一个
                g_mqttconfig.hosts[host_id].recentTopics.removeAt(i);
                g_mqttconfig.hosts[host_id].recentTopics.push_front(topic);
                saveConfig();
                return;
            }
        }
    }

    // 列表里不存在情况
    if(g_mqttconfig.hosts[host_id].recentTopics.size() >= 10)
        g_mqttconfig.hosts[host_id].recentTopics.pop_back();
    g_mqttconfig.hosts[host_id].recentTopics.push_front(topic);
    saveConfig();
}

void editSubscribeChecked(int host_id, int subs_id, bool checked)
{
    g_mqttconfig.hosts[host_id].subscribes[subs_id].checked = checked;
    saveConfig();
}

int findSubscribeByFullTopic(int host_id, const QString& fullTopic)
{
    auto &host = g_mqttconfig.hosts[host_id];
    QMap<QString, int> *map = &host.topic_subid_map;

    // 先查缓存
    auto it = map->find(fullTopic);
    if (it != map->end())
    {
        return it.value();
    }

    // 遍历订阅列表做通配符匹配
    const QVector<MqttSubscribe> &subs = host.subscribes;

    for (int i = 0; i < subs.size(); ++i)
    {
        const MqttSubscribe &sub = subs[i];

        if (matchTopic(sub.topic, fullTopic))
        {
            map->insert(fullTopic, i);

            return i;
        }
    }

    return -1;
}

void resetMessageCount(int host_id)
{
    for (auto& s : g_mqttconfig.hosts[host_id].subscribes)
    {
        s.msg_count = 0;
    }
}