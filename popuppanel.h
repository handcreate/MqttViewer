#ifndef POPUPPANEL_H
#define POPUPPANEL_H

#include <QWidget>

class QListWidget;

class PopupPanel : public QWidget
{
    Q_OBJECT
public:
    explicit PopupPanel(QWidget *parent = nullptr);

    void refresh();

signals:
    void categoryClicked(const QString& category);

private:
    QListWidget *listWidget;
};

#endif // POPUPPANEL_H