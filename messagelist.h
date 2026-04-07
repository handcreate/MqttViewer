#ifndef MESSAGELIST_H
#define MESSAGELIST_H

#include <QStandardItemModel>

#define DEFAULT_MESSAGE_LIMIT 100
#define LOAD_MORE_MESSAGE_COUNT 10
#define HISTORY_FILENAME "history_"

struct MqttMessage
{
    QString topic;
    QString content;
    QString time;
    bool isSend = false;
    int width;
    int height;
    QColor style = "#787878";

    MqttMessage() {}
};

class MessageList : public QObject
{
    Q_OBJECT

public:
    MessageList(int _host_id, QObject *parent = nullptr);

    void load(QString save_path);
    void save(QString save_path);
    void insert(const QString &topic, const QString &content, bool is_send);
    void clear();
    bool hasMore();
    void more();
    void displayRecent();
signals:
    void refreshSubscribeCount(int hostId, int subId);
    void refreshHostCount(int hostId);
public:
    QStandardItemModel *listModel;
    QVector<MqttMessage> list;
private:
    int host_id;
    bool modified;
};

#endif // MESSAGELIST_H
