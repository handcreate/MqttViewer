#ifndef MESSAGECENTER_H
#define MESSAGECENTER_H

#include <QObject>
#include <QMap>

class MessageCenter : public QObject
{
    Q_OBJECT
public:
    static MessageCenter& instance();

    void addMessage(const QString& category);
    void clear();

    QMap<QString, int> getCounts() const;

signals:
    void messageUpdated();

private:
    QMap<QString, int> categoryCount;
};

#endif // MESSAGECENTER_H
