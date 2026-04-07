#include "messagelist.h"
#include "data_processor.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>

MessageList::MessageList(int _host_id, QObject *parent)
    : host_id(_host_id)
    , modified(false)
    , listModel(new QStandardItemModel(parent))
{

}

void MessageList::load(QString save_path)
{
    QString historyFile = QDir(save_path).filePath(QString(HISTORY_FILENAME "%1.json").arg(host_id));
    if (!QFile::exists(historyFile))
        return;

    QFile file(historyFile);

    if (!file.open(QIODevice::ReadOnly))
    {
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject())
    {
        return;
    }

    list.clear();

    QJsonObject root = doc.object();

    for (auto msg : root["msgs"].toArray())
    {
        QJsonObject msg_jo = msg.toObject();

        MqttMessage message;
        message.topic = msg_jo["topic"].toString();
        message.content = msg_jo["content"].toString();
        message.time = msg_jo["time"].toString();
        message.isSend = msg_jo["is_send"].toBool();

        int subs_id = findSubscribeByFullTopic(host_id, message.topic);
        if(subs_id >= 0)
        {
            message.style = g_mqttconfig.hosts[host_id].subscribes[subs_id].color;
            g_mqttconfig.hosts[host_id].subscribes[subs_id].msg_count++;
        }

        list.append(message);
    }

    emit refreshHostCount(host_id);
}

void MessageList::save(QString save_path)
{
    if(!modified)
        return;

    QJsonObject root;

    QJsonArray msgArray;
    for (const auto &msg : list)
    {
        QJsonObject msg_jo;
        msg_jo["topic"] = msg.topic;
        msg_jo["content"] = msg.content;
        msg_jo["time"] = msg.time;
        msg_jo["is_send"] = msg.isSend;

        msgArray.append(msg_jo);
    }
    root["msgs"] = msgArray;

    QJsonDocument doc(root);

    QString historyFile = QDir(save_path).filePath(QString(HISTORY_FILENAME "%1.json").arg(host_id));
    QFile file(historyFile);
    if (!file.open(QIODevice::WriteOnly))
        return;

    file.write(doc.toJson(QJsonDocument::Indented));
}

void MessageList::insert(const QString &topic, const QString &content, bool is_send)
{
    QString time = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
    MqttMessage message;
    message.topic = topic;
    message.content = content;
    message.time = time;
    message.isSend = is_send;
    int subs_id = findSubscribeByFullTopic(host_id, message.topic);
    if(subs_id >= 0)
    {
        message.style = g_mqttconfig.hosts[host_id].subscribes[subs_id].color;
        g_mqttconfig.hosts[host_id].subscribes[subs_id].msg_count++;
        emit refreshSubscribeCount(host_id, subs_id);
    }
    list.append(message);

    modified = true;

    // 保持model里行数固定
    // if(listModel->rowCount() > DEFAULT_MESSAGE_LIMIT)
    //     listModel->removeRow(0);

    QStandardItem *item = new QStandardItem();
    item->setData(list.size()-1, Qt::DisplayRole);
    listModel->appendRow(item);
}

void MessageList::clear()
{
    list.clear();
    listModel->clear();
    resetMessageCount(host_id);
    modified = true;
    emit refreshHostCount(host_id);
}

bool MessageList::hasMore()
{
    return true;
}

void MessageList::more()
{

}

void MessageList::displayRecent()
{
    int start = 0;
    // if(list.size() > DEFAULT_MESSAGE_LIMIT)
    //     start = list.size() - DEFAULT_MESSAGE_LIMIT;

    listModel->clear();

    for(int i=start; i<list.size(); i++)
    {
        QStandardItem *item = new QStandardItem();
        item->setData(i, Qt::DisplayRole);
        listModel->appendRow(item);
    }
}