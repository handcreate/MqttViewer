#ifndef CHATINPUT_H
#define CHATINPUT_H

#include <QPlainTextEdit>
#include <QPushButton>

class ChatInput : public QPlainTextEdit
{
    Q_OBJECT
public:
    explicit ChatInput(QWidget *parent = nullptr);
    void refresh(int _host_id);

signals:
    void sendText(int host_id, QString text);

protected:
    void resizeEvent(QResizeEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;

private:
    void updateSendButtonPos();

private slots:
    void onSendClicked();

private:
    QPushButton *sendBtn;
    int host_id;
};

#endif // CHATINPUT_H