#include "mainwindow.h"
#include "ui_mainwindow.h"
// #include "json_highlighter.h"
#include "data_processor.h"
#include "hostdialog.h"
#include "subscribedialog.h"
#include "mqttconnection.h"
#include "messagelist.h"
#include "messagecenter.h"
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardItem>
#include <QMessageBox>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QScrollBar>
#include <QPainterPath>
#include <QTimer>
#include <QClipboard>
#include <QMenu>
#include <QElapsedTimer>

class MsgDelegate : public QStyledItemDelegate
{
public:
    QString buildHtml(const QString &topic,
                      const QString &msg,
                      const QString &time) const
    {
        QString html;

        html += "<div style='font-weight:bold; color:#2c7be5; margin-bottom:6px;'>"
                + topic.toHtmlEscaped() + "</div>";

        html += "<div style='margin-bottom:10px; white-space:pre-wrap;'>"
                + msg.toHtmlEscaped() + "</div>";

        html += "<div style='color:gray; font-size:12px; text-align:right;'>"
                + time.toHtmlEscaped() + "</div>";

        return html;
    }

    QSize sizeHint(const QStyleOptionViewItem &option,
        const QModelIndex &index) const override
    {
        int width = option.rect.width();
        if (width <= 0)
        {
            auto view = qobject_cast<const QListView*>(option.widget);
            if (view)
                width = view->viewport()->width();
            else
                width = 300;
        }

        int margin  = 8;
        int spacing = 4;

        int topicHeight = 20;
        int msgHeight = 60;
        int timeHeight = 20;

        int maxBubbleWidth = width * 0.7;

        uint list_id = index.data(Qt::DisplayRole).toUInt();
        auto &m = g_mqttconfig.hosts[g_mqttconfig.host_id]
                      .msglist->list[list_id];

        if (index == expandedIndex)
        {
            QTextDocument doc;

            QString html = buildHtml(m.topic, m.content, m.time);

            doc.setHtml(html);
            doc.setTextWidth(maxBubbleWidth - 12);

            int total = doc.size().height();

            int bubbleHeight = total + 16; // 内边距

            return QSize(width, bubbleHeight + 16);
        }

        int bubbleHeight =
            margin + topicHeight +
            spacing + msgHeight +
            spacing + timeHeight +
            margin;
        int totalHeight = bubbleHeight + 2 * margin;

        return QSize(width, totalHeight);
    }

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        painter->save();

        QRect rect = option.rect;

        uint list_id = index.data(Qt::DisplayRole).toUInt();
        auto &mqttmsg = g_mqttconfig.hosts[g_mqttconfig.host_id]
                            .msglist->list[list_id];

        QString msg   = mqttmsg.content;
        QString topic = mqttmsg.topic;
        QString time  = mqttmsg.time;
        bool is_send  = mqttmsg.isSend;
        QColor styleColor = mqttmsg.style;

        int margin  = 8;
        int spacing = 4;
        int innerMargin = 6;

        int maxBubbleWidth = rect.width() * 0.7;

        int topicHeight = 20;
        int msgHeight   = 60;
        int timeHeight  = 20;

        // ===== 默认高度 =====
        int bubbleHeight =
            margin + topicHeight +
            spacing + msgHeight +
            spacing + timeHeight +
            margin;

        // ===== ⭐展开：重新计算高度 =====
        QTextDocument doc;
        if (index == expandedIndex)
        {
            QString html = buildHtml(topic, msg, time);

            doc.setHtml(html);
            doc.setTextWidth(maxBubbleWidth - innerMargin * 2);

            int docHeight = doc.size().height();

            bubbleHeight = innerMargin + docHeight + innerMargin;
        }

        // ===== 气泡位置 =====
        int bubbleX = is_send
                          ? rect.right() - margin - maxBubbleWidth
                          : rect.left() + margin;

        QRect bubbleRect(bubbleX,
                         rect.top() + margin,
                         maxBubbleWidth,
                         bubbleHeight);

        // ===== 背景 =====
        painter->setRenderHint(QPainter::Antialiasing);

        QColor bubbleColor = is_send
                                 ? QColor(52, 195, 136)
                                 : QColor(240, 240, 240);

        QPainterPath path;
        path.addRoundedRect(bubbleRect, 8, 8);
        painter->fillPath(path, bubbleColor);

        // 顶部色条
        if (!is_send)
        {
            QRect barRect(bubbleRect.left(),
                          bubbleRect.top(),
                          bubbleRect.width(),
                          4);

            painter->fillRect(barRect, styleColor);
        }

        // ===== 内容区域 =====
        int y = bubbleRect.top() + innerMargin;

        QRect topicRect(bubbleRect.left() + innerMargin, y,
                        bubbleRect.width() - 2 * innerMargin, topicHeight);
        y += topicHeight + spacing;

        QRect msgRect(bubbleRect.left() + innerMargin, y,
                      bubbleRect.width() - 2 * innerMargin, msgHeight);
        y += msgHeight + spacing;

        QRect timeRect(bubbleRect.left() + innerMargin, y,
                       bubbleRect.width() - 2 * innerMargin, timeHeight);

        // ===== ⭐绘制 =====
        if (index == expandedIndex)
        {
            // ✅ 从 topicRect 开始
            QPoint start = topicRect.topLeft();

            painter->save();
            painter->translate(start);
            doc.drawContents(painter);
            painter->restore();
        }
        else
        {
            painter->setPen(Qt::blue);
            painter->drawText(topicRect, Qt::TextSingleLine, topic);

            painter->setPen(Qt::black);
            painter->drawText(msgRect, Qt::TextWordWrap, msg);

            painter->setPen(Qt::gray);
            painter->drawText(timeRect, Qt::AlignRight, time);
        }

        painter->restore();
    }

public:
    QModelIndex expandedIndex;   // 当前展开项

    void setExpandedIndex(const QModelIndex &index)
    {
        expandedIndex = index;
    }
};

QIcon makeColorIcon(const QColor& color)
{
    QPixmap pix(12, 12);
    pix.fill(color);
    return QIcon(pix);
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_treeModel(new QStandardItemModel())
    , m_listModel(nullptr)
    , icon_green(":/icons/icons/ball_green.ico")
    , icon_gray(":/icons/icons/ball_gray.ico")
{
    ui->setupUi(this);

    setMinimumSize(800,600);
    ui->hSplitter->setStretchFactor(0, 0); // 左边
    ui->hSplitter->setStretchFactor(1, 1); // 右边整体
    ui->vSplitter->setStretchFactor(0, 1); // 右上 ✅
    ui->vSplitter->setStretchFactor(1, 0); // 右下
    ui->leftWidget->setMinimumWidth(200);
    ui->sendWidget->setMinimumHeight(100);
    ui->hSplitter->setSizes({240, 560});        // 左右初始比例
    ui->vSplitter->setSizes({400, 200});       // 上下初始比例
    ui->vSplitter->setChildrenCollapsible(false);

    ui->sendWidget->setStyleSheet(R"(
        QWidget {
            padding-right: 1px;
            padding-bottom: 1px;
        }
    )");

    // ui->splitter->setHandleWidth(1);
    setStyleSheet(R"(
        QSplitter::handle {
            width: 1px;
            background-color: #cccccc;
        }

        QSplitter::handle:hover {
            background-color: #999999;
        }
    )");

    // 等宽字体（关键）
    // QFont font("Consolas");
    // font.setStyleHint(QFont::Monospace);
    // ui->plainTextEdit->setFont(font);
    // 不自动换行（JSON推荐）
    // ui->plainTextEdit->setLineWrapMode(QPlainTextEdit::NoWrap);
    // json文本高亮
    // new JsonHighlighter(ui->plainTextEdit->document());

    // 读取配置文件
    loadConfig();
    QString cachePath = getCachePath();
    qDebug() << cachePath;
    for(int i = 0; i < g_mqttconfig.hosts.size(); i++)
    {
        auto &host = g_mqttconfig.hosts[i];
        host.msglist = new MessageList(i, this);
        connect(host.msglist, &MessageList::refreshHostCount,
                this, &MainWindow::displayHostCount);
        connect(host.msglist, &MessageList::refreshSubscribeCount,
                this, &MainWindow::displaySubscribeCount);
    }

    initTreeView();

    ui->treeView->setModel(m_treeModel);
    ui->treeView->setHeaderHidden(true);
    ui->treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->treeView->setItemsExpandable(false); // 禁止用户展开或折叠操作
    ui->treeView->expandAll();
    // 去掉左侧“小三角”
    ui->treeView->setStyleSheet(
        "QTreeView::branch { image: none; }"
        );
    // ui->treeView->setIndentation(0);
    ui->treeView->setRootIsDecorated(false); // 去掉最外层装饰
    connect(m_treeModel, &QStandardItemModel::itemChanged,
            this, &MainWindow::onTreeCheckItemChanged);
    connect(ui->treeView, &QTreeView::customContextMenuRequested,
        this, &MainWindow::onTreeContextMenu);
    connect(ui->treeView, &QTreeView::clicked,
            this, &MainWindow::onTreeClicked);
    connect(ui->treeView, &QTreeView::doubleClicked,
        this, &MainWindow::onTreeDoubleClicked);

    // ui->listView->setModel(listModel);

    msgDelegate = new MsgDelegate();
    ui->listView->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->listView->setItemDelegate(msgDelegate);
    // ui->listView->setLayoutMode(QListView::Batched);
    // ui->listView->setUniformItemSizes(false);
    ui->listView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    ui->listView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    connect(ui->listView, &QListView::doubleClicked,
            this, &MainWindow::onListDoubleClicked);
    connect(ui->listView, &QListView::customContextMenuRequested,
            this, &MainWindow::onListViewMenu);
    connect(ui->listView, &QListView::clicked,
            this, [=](const QModelIndex &index)
            {
                if (msgDelegate->expandedIndex == index)
                    msgDelegate->setExpandedIndex(QModelIndex()); // 收起
                else
                    msgDelegate->setExpandedIndex(index);

                // 强制刷新
                ui->listView->doItemsLayout();
            });

    connect(ui->sendTextEdit, &ChatInput::sendText,
        this, &MainWindow::sendText);

    ui->sendTextEdit->setEnabled(false);

    // 托盘图标
    trayIcon = new QSystemTrayIcon(this);
    icon_tray = QIcon(":/icons/icons/app.ico");
    trayIcon->setIcon(icon_tray);

    // 托盘菜单
    trayMenu = new QMenu(this);

    QAction *showAction = new QAction("显示", this);
    QAction *quitAction = new QAction("退出", this);

    trayMenu->addAction(showAction);
    trayMenu->addAction(quitAction);

    trayIcon->setContextMenu(trayMenu);

    // 信号
    connect(showAction, &QAction::triggered, this, [=]() {
        this->show();
        this->raise();
        this->activateWindow();
    });
    connect(quitAction, &QAction::triggered, []() {
        QApplication::quit();
    });
    panel = new PopupPanel();
    connect(trayIcon, &QSystemTrayIcon::activated, this,
            [=](QSystemTrayIcon::ActivationReason reason){
                if (reason == QSystemTrayIcon::Trigger)
                {
                    // 如果托盘图片在闪烁，显示消息提醒面板
                    if(trayBlinkTimer->isActive())
                    {
                        panel->refresh();

                        // ⭐ 强制计算布局尺寸
                        panel->adjustSize();

                        QPoint pos = QCursor::pos();
                        panel->move(pos.x() - panel->width() / 2,
                                      pos.y() - panel->height() - 20);

                        panel->show();

                        stopTrayBlink();
                    }
                    // 没有，则显示主界面
                    else
                    {
                        this->show();
                        this->raise();
                        this->activateWindow();
                    }
                }
            });
    connect(panel, &PopupPanel::categoryClicked, this, &MainWindow::trayPanelClicked);

    trayIcon->show();

    // 定时器
    trayBlinkTimer = new QTimer(this);
    trayBlinkTimer->setInterval(500); // 500ms闪烁
    connect(trayBlinkTimer, &QTimer::timeout, this, [=]() {
        blinkState = !blinkState;
        trayIcon->setIcon(blinkState ? icon_empty : icon_tray);
    });
    connect(&MessageCenter::instance(), &MessageCenter::messageUpdated, this, [=]() {
        startTrayBlink();
    });

    for(int i = 0; i < g_mqttconfig.hosts.size(); i++)
    {
        auto &host = g_mqttconfig.hosts[i];
        host.msglist->load(cachePath);
    }
}

MainWindow::~MainWindow()
{
    QString cachePath = getCachePath();
    for(int i = 0; i < g_mqttconfig.hosts.size(); i++)
    {
        auto &host = g_mqttconfig.hosts[i];
        host.msglist->save(cachePath);
    }

    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    hide();
    event->ignore();
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::ActivationChange)
    {
        if (this->isActiveWindow())
        {
            // 窗口被激活
            if (trayBlinkTimer->isActive())
                stopTrayBlink();
        }
    }

    QMainWindow::changeEvent(event);
}

void MainWindow::initTreeView()
{
    QStandardItem* root = m_treeModel->invisibleRootItem();

    for(int i = 0; i < g_mqttconfig.hosts.size(); i++)
    {
        auto &host = g_mqttconfig.hosts[i];
        bool is_conn = false;
        if(host.connection)
            is_conn = host.connection->isConnected;

        QStandardItem* item = new QStandardItem(host.name);
        item->setData(i, Qt::UserRole + 1); // host id in vector
        item->setData(-1, Qt::UserRole + 2); // subscribe id in vector
        item->setIcon(is_conn ? icon_green : icon_gray);
        for(int j = 0; j < host.subscribes.size(); j++)
        {
            auto &subscribe = host.subscribes[j];
            QStandardItem* subitem = new QStandardItem(subscribe.name);
            subitem->setData(i, Qt::UserRole + 1); // host id in vector
            subitem->setData(j, Qt::UserRole + 2); // subscribe id in vector
            subitem->setIcon(makeColorIcon(QColor(subscribe.color)));
            subitem->setCheckable(true);
            subitem->setCheckState(host.subscribes[j].checked ? Qt::Checked : Qt::Unchecked);
            item->appendRow(subitem);
        }
        root->appendRow(item);
    }
}

QStandardItem* MainWindow::findItemInTreeModel(int hostId, int subId)
{
    QStandardItem* root = m_treeModel->invisibleRootItem();

    for (int i = 0; i < root->rowCount(); ++i)
    {
        QStandardItem* hostItem = root->child(i);

        int hId = hostItem->data(Qt::UserRole + 1).toInt();
        int sId = hostItem->data(Qt::UserRole + 2).toInt();

        // 如果找的是 host（subId == -1）
        if (hId == hostId && sId == subId)
            return hostItem;

        // 遍历 subscribe
        for (int j = 0; j < hostItem->rowCount(); ++j)
        {
            QStandardItem* subItem = hostItem->child(j);

            int h = subItem->data(Qt::UserRole + 1).toInt();
            int s = subItem->data(Qt::UserRole + 2).toInt();

            if (h == hostId && s == subId)
                return subItem;
        }
    }

    return nullptr;
}

void MainWindow::onTreeContextMenu(const QPoint &pos)
{
    QModelIndex index = ui->treeView->indexAt(pos);

    // 没点到节点（空白区域）
    if (!index.isValid())
    {
        QMenu menu;
        QAction* addHostAction = new QAction("新建连接");
        menu.addAction(addHostAction);
        // 点击事件
        connect(addHostAction, &QAction::triggered, [=]() {
            addHost();
        });
        menu.exec(ui->treeView->viewport()->mapToGlobal(pos));

        return;
    }

    QString name = index.data().toString();
    int host_id = index.data(Qt::UserRole + 1).toInt();
    int subscribe_id = index.data(Qt::UserRole + 2).toInt();

    // 点击到连接上
    if(subscribe_id < 0)
    {
        bool is_conn = false;
        std::shared_ptr<MqttConnection> conn = g_mqttconfig.hosts[host_id].connection;
        if(conn && conn->isConnected)
        {
            is_conn = true;
        }

        QMenu menu;

        QAction* toggleHostAction = new QAction(is_conn ? "关闭连接" : "打开连接");
        QAction* editHostAction = new QAction("修改连接");
        QAction* deleteHostAction = new QAction("删除连接");
        QAction* addSubscribeAction = new QAction("新建订阅");

        menu.addAction(toggleHostAction);
        menu.addSeparator();
        menu.addAction(editHostAction);
        menu.addAction(deleteHostAction);
        menu.addSeparator();
        menu.addAction(addSubscribeAction);

        if(is_conn)
        {
            deleteHostAction->setEnabled(false);
        }

        connect(toggleHostAction, &QAction::triggered, [=]() {
            toggleHost(host_id);
        });

        connect(editHostAction, &QAction::triggered, [=]() {
            editHost(host_id);
        });

        connect(deleteHostAction, &QAction::triggered, [=]() {
            deleteHost(host_id, name);
        });

        connect(addSubscribeAction, &QAction::triggered, [=]() {
            addSubscribe(host_id);
        });

        // 弹出菜单（关键）
        menu.exec(ui->treeView->viewport()->mapToGlobal(pos));
    }
    else // 点击到订阅上
    {
        QMenu menu;

        QAction* editSubscribeAction = new QAction("修改订阅");
        QAction* deleteSubscribection = new QAction("删除订阅");

        menu.addAction(editSubscribeAction);
        menu.addAction(deleteSubscribection);
        menu.addSeparator();

        connect(editSubscribeAction, &QAction::triggered, [=]() {
            editSubscribe(host_id, subscribe_id);
        });

        connect(deleteSubscribection, &QAction::triggered, [=]() {
            deleteSubscribe(host_id, subscribe_id, name);
        });

        // 弹出菜单（关键）
        menu.exec(ui->treeView->viewport()->mapToGlobal(pos));
    }
}

void MainWindow::onTreeClicked(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    QString name = index.data().toString();
    int host_id = index.data(Qt::UserRole + 1).toInt();
    int subscribe_id = index.data(Qt::UserRole + 2).toInt();
    qDebug() << "clicked" << name << " host_id: " << host_id << " sub_id: " << subscribe_id;

    changeListViewModel(host_id);
    refreshSendWidget(host_id);
}

void MainWindow::onTreeDoubleClicked(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    QString name = index.data().toString();
    int host_id = index.data(Qt::UserRole + 1).toInt();
    int subscribe_id = index.data(Qt::UserRole + 2).toInt();
    qDebug() << "double" << name << " host_id: " << host_id << " sub_id: " << subscribe_id;

    if(subscribe_id < 0)
        toggleHost(host_id);
}

void MainWindow::onTreeCheckItemChanged(QStandardItem* item)
{
    if (item->isCheckable())
    {
        QString name = item->data(Qt::DisplayRole).toString();
        int host_id = item->data(Qt::UserRole + 1).toInt();
        int subscribe_id = item->data(Qt::UserRole + 2).toInt();

        editSubscribeChecked(host_id, subscribe_id, (item->checkState() == Qt::Checked));

        std::shared_ptr<MqttConnection> conn = g_mqttconfig.hosts[host_id].connection;
        if(conn && conn->isConnected)
        {
            MqttSubscribe &sub = g_mqttconfig.hosts[host_id].subscribes[subscribe_id];
            if (item->checkState() == Qt::Checked)
            {
                qDebug() << "选中：" << name << " host_id: " << host_id << " sub_id: " << subscribe_id;
                conn->subscribe(sub.topic, sub.qos);
            }
            else
            {
                qDebug() << "取消选中：" << name << " host_id: " << host_id << " sub_id: " << subscribe_id;
                conn->unsubscribe(sub.topic);
            }
        }
    }
}

void MainWindow::sendText(int host_id, const QString &msg)
{
    QString topic = ui->sendTopicBox->currentText().trimmed();
    if(topic.isEmpty())
        return;
    int qos = ui->sendQosBox->currentIndex();

    std::shared_ptr<MqttConnection> conn = g_mqttconfig.hosts[host_id].connection;
    if(conn && conn->isConnected)
    {
        bool res = conn->publish(topic, msg, qos);
        if(!res)
            return;

        g_mqttconfig.hosts[host_id].msglist->insert(topic, msg, true);

        addRecentTopics(host_id, topic, msg);
        refreshSendWidget(host_id);
    }
}

void MainWindow::onListDoubleClicked(const QModelIndex &index)
{
    // QString msg   = index.data(Qt::DisplayRole).toString();
    // QString topic = index.data(Qt::UserRole).toString();
    // QString time  = index.data(Qt::UserRole + 1).toString();

    // DetailDialog *dlg = new DetailDialog(this, topic, msg, time);
    // dlg->resize(400, 300);

    // // ===== 1. item 在全局的位置 =====
    // QRect itemRect = ui->listView->visualRect(index);
    // QPoint globalPos = ui->listView->viewport()->mapToGlobal(itemRect.topLeft());

    // int x = globalPos.x();
    // int y = globalPos.y();

    // int dlgW = dlg->width();
    // int dlgH = dlg->height();

    // // ===== 2. ListView范围（限制左上不要飞出去）=====
    // QRect listRect = ui->listView->viewport()->rect();
    // QPoint listTopLeft = ui->listView->viewport()->mapToGlobal(listRect.topLeft());
    // // QPoint listBottomRight = ui->listView->viewport()->mapToGlobal(listRect.bottomRight());

    // // 👉 上边界：不能高于 listView
    // if (y < listTopLeft.y())
    //     y = listTopLeft.y();

    // // 👉 左边界（可选）
    // if (x < listTopLeft.x())
    //     x = listTopLeft.x();

    // // ===== 3. MainWindow范围（限制底部不能超出）=====
    // QRect winRect = this->rect();
    // // QPoint winTopLeft = this->mapToGlobal(winRect.topLeft());
    // QPoint winBottomRight = this->mapToGlobal(winRect.bottomRight());

    // // 👉 下边界：弹窗底不能超出 MainWindow
    // if (y + dlgH > winBottomRight.y())
    //     y = winBottomRight.y() - dlgH;

    // // 👉 右边界（顺手处理一下）
    // if (x + dlgW > winBottomRight.x())
    //     x = winBottomRight.x() - dlgW;

    // // ===== 4. 应用位置 =====
    // dlg->move(x, y);
    // dlg->show();
}

void MainWindow::onListViewMenu(const QPoint &pos)
{
    QModelIndex index = ui->listView->indexAt(pos);

    QMenu menu;

    QAction *copyAction = menu.addAction("复制消息");
    QAction *clearAction = menu.addAction("清空列表");

    QAction *selected = menu.exec(ui->listView->viewport()->mapToGlobal(pos));

    if (selected == copyAction && index.isValid())
    {
        uint list_id = index.data(Qt::DisplayRole).toUInt();

        auto &msgObj = g_mqttconfig.hosts[g_mqttconfig.host_id]
                           .msglist->list[list_id];

        QString text;

        text += "Topic: " + msgObj.topic + "\n";
        text += "Time:  " + msgObj.time  + "\n";
        text += "--------------------------------\n";
        text += msgObj.content;

        QClipboard *clipboard = QApplication::clipboard();
        clipboard->setText(text);
    }
    else if (selected == clearAction)
    {
        g_mqttconfig.hosts[g_mqttconfig.host_id].msglist->clear();
    }
}

void MainWindow::addHost()
{
    HostDialog dlg(this, false);
    if(dlg.exec() == QDialog::Accepted)
    {
        MqttHost edit_host = dlg.getData();
        int host_id = addHostConfig(edit_host);

        auto &host = g_mqttconfig.hosts[host_id];
        host.msglist = new MessageList(host_id, this);
        connect(host.msglist, &MessageList::refreshHostCount,
                this, &MainWindow::displayHostCount);
        connect(host.msglist, &MessageList::refreshSubscribeCount,
                this, &MainWindow::displaySubscribeCount);

        // 更新treeview
        QStandardItem* root = m_treeModel->invisibleRootItem();
        QStandardItem* item = new QStandardItem(host.name);
        item->setData(host_id, Qt::UserRole + 1); // host id in vector
        item->setData(-1, Qt::UserRole + 2); // subscribe id in vector
        item->setIcon(icon_gray);
        root->appendRow(item);
        // 展开
        QModelIndex index = m_treeModel->indexFromItem(item);
        ui->treeView->expand(index);
    }
}

void MainWindow::editHost(int host_id)
{
    HostDialog dlg(this, true);
    dlg.setData(g_mqttconfig.hosts[host_id]);
    if(dlg.exec() == QDialog::Accepted)
    {
        MqttHost host = dlg.getData();
        editHostConfig(host_id, host);
    }
}

void MainWindow::deleteHost(int host_id, QString name)
{
    QMessageBox::StandardButton reply;

    reply = QMessageBox::question(
        this,
        "确认删除",
        QString("确定要删除连接 %1 吗？").arg(name),
        QMessageBox::Yes | QMessageBox::No
        );

    if (reply == QMessageBox::Yes)
    {
        deleteHostConfig(host_id);

        // 更新treeview
        QStandardItem* root = m_treeModel->invisibleRootItem();
        for (int i = 0; i < root->rowCount(); ++i)
        {
            QStandardItem* item = root->child(i);
            if (item->data(Qt::UserRole + 1).toInt() == host_id)
            {
                root->removeRow(i);
                break;
            }
        }
    }
}

void MainWindow::addSubscribe(int host_id)
{
    SubscribeDialog dlg(this, host_id, false);
    if(dlg.exec() == QDialog::Accepted)
    {
        MqttSubscribe subs = dlg.getData();
        addSubscribeConfig(host_id, subs);

        // 更新treeview
        QStandardItem* treeItem = findItemInTreeModel(host_id);
        QStandardItem* subitem = new QStandardItem(subs.name);
        subitem->setData(host_id, Qt::UserRole + 1); // host id in vector
        int sub_id = g_mqttconfig.hosts[host_id].subscribes.size() - 1;
        subitem->setData(sub_id, Qt::UserRole + 2); // subscribe id in vector
        subitem->setCheckable(true);
        subitem->setIcon(makeColorIcon(QColor(subs.color)));
        subitem->setCheckState(Qt::Unchecked);
        treeItem->appendRow(subitem);
    }
}

void MainWindow::editSubscribe(int host_id, int subs_id)
{
    SubscribeDialog dlg(this, host_id, true);
    dlg.setData(g_mqttconfig.hosts[host_id].subscribes[subs_id]);
    if(dlg.exec() == QDialog::Accepted)
    {
        MqttSubscribe subs = dlg.getData();
        editSubscribeConfig(host_id, subs_id, subs);

        // 更新treeview
        QStandardItem* treeItem = findItemInTreeModel(host_id, subs_id);
        treeItem->setIcon(makeColorIcon(QColor(subs.color)));
    }
}

void MainWindow::deleteSubscribe(int host_id, int subs_id, QString name)
{
    QMessageBox::StandardButton reply;

    reply = QMessageBox::question(
        this,
        "确认删除",
        QString("确定要删除订阅 %1 吗？").arg(name),
        QMessageBox::Yes | QMessageBox::No
        );

    if (reply == QMessageBox::Yes)
    {
        deleteSubscribeConfig(host_id, subs_id);

        // 更新treeview
        QStandardItem* hostItem = findItemInTreeModel(host_id);
        if (!hostItem) return;

        for (int i = 0; i < hostItem->rowCount(); ++i)
        {
            QStandardItem* subItem = hostItem->child(i);
            if (subItem->data(Qt::UserRole + 2).toInt() == subs_id)
            {
                hostItem->removeRow(i);
                break;
            }
        }
    }
}

void MainWindow::toggleHost(int host_id)
{
    if(!g_mqttconfig.hosts[host_id].connection)
        g_mqttconfig.hosts[host_id].connection = std::make_shared<MqttConnection>(host_id);

    std::shared_ptr<MqttConnection> conn = g_mqttconfig.hosts[host_id].connection;
    bool is_conn = conn->isConnected;
    if(!is_conn)
    {
        conn->open();
    }
    else
    {
        // 将引用值为空，会引发conn对象析构，从而自动调用conn的close
        g_mqttconfig.hosts[host_id].connection = nullptr;
    }

    refreshSendWidget(host_id);

    QStandardItem* treeItem = findItemInTreeModel(host_id);
    treeItem->setIcon(!is_conn ? icon_green : icon_gray);
}

void  MainWindow::changeListViewModel(int host_id)
{
    if(g_mqttconfig.host_id == host_id)
        return;

    g_mqttconfig.host_id = host_id;
    if (m_listModel)
    {
        disconnect(m_listModel, nullptr, ui->listView, nullptr);
    }
    m_listModel = g_mqttconfig.hosts[host_id].msglist->listModel;
    ui->listView->setModel(m_listModel);
    g_mqttconfig.hosts[host_id].msglist->displayRecent();
    QTimer::singleShot(0, ui->listView, [=]() {
        ui->listView->scrollToBottom();
    });
    connect(m_listModel, &QStandardItemModel::rowsInserted,
            ui->listView, [=](const QModelIndex &, int, int){
                qDebug() << "rowsInserted";
                QScrollBar* bar = ui->listView->verticalScrollBar();

                // 如果原本就在底部 → 保持在底部
                if (bar->value() >= bar->maximum() - 2)
                {
                    ui->listView->scrollToBottom();
                }
            });
}

void MainWindow::refreshSendWidget(int host_id)
{
    ui->sendTextEdit->refresh(host_id);
    ui->sendTopicBox->clear();
    QStringList list = QStringList::fromVector(g_mqttconfig.hosts[host_id].recentTopics);
    ui->sendTopicBox->addItems(list);
}

void MainWindow::displaySubscribeCount(int hostId, int subId)
{
    QStandardItem* subItem = findItemInTreeModel(hostId, subId);
    MqttSubscribe &subs = g_mqttconfig.hosts[hostId].subscribes[subId];
    subItem->setText(QString("%1 [%2]").arg(subs.name).arg(subs.msg_count));

    if((!this->isActiveWindow()) && subs.remind)
        MessageCenter::instance().addMessage(QString("[%1]%2").arg(g_mqttconfig.hosts[hostId].name).arg(subs.name));
}

void MainWindow::displayHostCount(int hostId)
{
    QStandardItem* hostItem = findItemInTreeModel(hostId);
    if (!hostItem) return;

    for (int i = 0; i < hostItem->rowCount(); ++i)
    {
        QStandardItem* subItem = hostItem->child(i);
        MqttSubscribe &subs = g_mqttconfig.hosts[hostId].subscribes[i];
        subItem->setText(QString("%1 [%2]").arg(subs.name).arg(subs.msg_count));
    }
}

void MainWindow::startTrayBlink()
{
    if (!trayBlinkTimer->isActive())
        trayBlinkTimer->start();
}

void MainWindow::stopTrayBlink()
{
    trayBlinkTimer->stop();
    trayIcon->setIcon(icon_tray);
    MessageCenter::instance().clear();
}

void MainWindow::trayPanelClicked(const QString& category)
{
    MessageCenter::instance().clear();
    this->show();
    this->raise();
    this->activateWindow();
}