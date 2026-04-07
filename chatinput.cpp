#include "chatinput.h"
#include "data_processor.h"
#include "mqttconnection.h"
#include <QScrollBar>
#include <QTimer>

ChatInput::ChatInput(QWidget *parent)
    : QPlainTextEdit(parent)
    , host_id(-1)
{
    sendBtn = new QPushButton(this);

    QIcon icon;
    icon.addFile(":/icons/icons/send.png", QSize(), QIcon::Normal);
    icon.addFile(":/icons/icons/send_gray.png", QSize(), QIcon::Disabled);
    sendBtn->setIcon(icon);
    sendBtn->setIconSize(QSize(24, 24));
    sendBtn->setFixedSize(30, 30);

    sendBtn->setStyleSheet(R"(
        QPushButton {
            border: none;
            background: transparent;
        }
        QPushButton:hover {
            background-color: rgba(0,0,0,30);
            border-radius: 15px;
        }
    )");

    setStyleSheet(R"(
        QPlainTextEdit QScrollBar:vertical {
            width: 6px;              /* 👈 关键：宽度 */
            background: transparent;
        }

        QPlainTextEdit QScrollBar::handle:vertical {
            background: rgba(0,0,0,80);
            border-radius: 3px;
            min-height: 20px;
        }

        QPlainTextEdit QScrollBar::handle:vertical:hover {
            background: rgba(0,0,0,120);
        }

        QPlainTextEdit QScrollBar::add-line:vertical,
        QPlainTextEdit QScrollBar::sub-line:vertical {
            height: 0px;
        }
    )");

    connect(sendBtn, &QPushButton::clicked, this, &ChatInput::onSendClicked);

    // setViewportMargins(0, 0, 5, 5);

    QTimer::singleShot(0, this, [this]() {
        updateSendButtonPos();
    });

    // 默认情况下失效
    QPlainTextEdit::setEnabled(false);
    sendBtn->setEnabled(false);
}

void ChatInput::updateSendButtonPos()
{
    QRect r = viewport()->geometry();

    int margin = 5;
    int x = r.right() - sendBtn->width() - margin;
    int y = r.bottom() - sendBtn->height() - margin;

    sendBtn->move(x, y);
}

void ChatInput::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);
    updateSendButtonPos();
}

void ChatInput::keyPressEvent(QKeyEvent *e)
{
    if ((e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) &&
        !(e->modifiers() & Qt::ShiftModifier))
    {
        onSendClicked();
        return;
    }

    QPlainTextEdit::keyPressEvent(e);
}

void ChatInput::onSendClicked()
{
    QString text = toPlainText().trimmed();
    if (!text.isEmpty() && host_id >= 0)
    {
        emit sendText(host_id, text);
        // clear();
    }
}

void ChatInput::refresh(int _host_id)
{
    host_id = _host_id;
    QString text = toPlainText().trimmed();
    if (text.isEmpty())
    {
        setPlainText(g_mqttconfig.hosts[host_id].recentSendMessage);
    }

    bool is_conn = false;
    if(g_mqttconfig.hosts[host_id].connection)
    {
        is_conn = g_mqttconfig.hosts[host_id].connection->isConnected;
    }

    QPlainTextEdit::setEnabled(is_conn);
    sendBtn->setEnabled(is_conn);
}