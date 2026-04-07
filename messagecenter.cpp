#include "messagecenter.h"

MessageCenter& MessageCenter::instance()
{
    static MessageCenter inst;
    return inst;
}

void MessageCenter::addMessage(const QString& category)
{
    categoryCount[category]++;

    emit messageUpdated();
}

void MessageCenter::clear()
{
    categoryCount.clear();
}

QMap<QString, int> MessageCenter::getCounts() const
{
    return categoryCount;
}
