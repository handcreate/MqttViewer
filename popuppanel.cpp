#include "popuppanel.h"
#include "messagecenter.h"

#include <QListWidget>
#include <QVBoxLayout>

PopupPanel::PopupPanel(QWidget *parent)
    : QWidget(nullptr)
{
    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
    setFixedWidth(200);

    listWidget = new QListWidget(this);
    listWidget->setStyleSheet(R"(
        QListWidget::item {
            padding-top: 4px;
            padding-bottom: 4px;
        }
    )");

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(listWidget);
    layout->setContentsMargins(5, 10, 5, 10);

    // 点击事件
    connect(listWidget, &QListWidget::itemClicked, this,
            [=](QListWidgetItem *item){
                QString category = item->data(Qt::UserRole).toString();
                close();

                emit categoryClicked(category);
            });
}

void PopupPanel::refresh()
{
    listWidget->clear();

    auto counts = MessageCenter::instance().getCounts();

    int itemHeight = 28;   // 每一项高度（可调）
    int maxVisibleItems = 6;  // 最多显示6条（像微信）

    int total = 0;

    for (auto it = counts.begin(); it != counts.end(); ++it)
    {
        QString category = it.key();
        int count = it.value();

        QString text = QString("%1 (%2)").arg(category).arg(count);

        QListWidgetItem *item = new QListWidgetItem(text);
        item->setData(Qt::UserRole, category);

        listWidget->addItem(item);

        total++;
    }

    // ⭐ 核心：计算高度
    int visibleCount = qMin(total, maxVisibleItems);

    int height = visibleCount * itemHeight + 20; // + padding

    setFixedHeight(height);

    // ⭐ 是否启用滚动条
    if (total > maxVisibleItems)
    {
        listWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    }
    else
    {
        listWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    }

    adjustSize();  // ⭐ 必须
}