#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStandardItemModel>
#include <QSystemTrayIcon>
#include <QMenu>
#include "data_processor.h"
#include "popuppanel.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MsgDelegate;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;

private:
    void initTreeView();
    // void refreshTreeView();
    void onTreeContextMenu(const QPoint &pos);
    void onTreeClicked(const QModelIndex &index);
    void onTreeDoubleClicked(const QModelIndex &index);
    void onTreeCheckItemChanged(QStandardItem* item);
    void sendText(int host_id, const QString &msg);
    void onListDoubleClicked(const QModelIndex &index);
    void onListViewMenu(const QPoint &pos);
    void refreshSendWidget(int host_id);
    void changeListViewModel(int host_id);
    void addHost();
    void editHost(int host_id);
    void deleteHost(int host_id, QString name);
    void addSubscribe(int host_id);
    void editSubscribe(int host_id, int subs_id);
    void deleteSubscribe(int host_id, int subs_id, QString name);
    void toggleHost(int host_id);
    QStandardItem* findItemInTreeModel(int hostId, int subId = -1);
    void displaySubscribeCount(int hostId, int subId);
    void displayHostCount(int hostId);
    void startTrayBlink();
    void stopTrayBlink();
    void trayPanelClicked(const QString& category);

private:
    Ui::MainWindow *ui;
    MqttConfig mqttConfig;
    QStandardItemModel *m_treeModel;
    QStandardItemModel *m_listModel;
    MsgDelegate *msgDelegate;

    QIcon icon_green;
    QIcon icon_gray;
    QSystemTrayIcon *trayIcon;
    QMenu *trayMenu;
    QTimer *trayBlinkTimer;
    QIcon icon_tray;
    QIcon icon_empty;
    bool blinkState = false;
    PopupPanel *panel;
};
#endif // MAINWINDOW_H
