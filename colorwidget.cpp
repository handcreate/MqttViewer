#include "colorwidget.h"

#include <QPainter>
#include <QMouseEvent>
#include <QColorDialog>

ColorWidget::ColorWidget(QWidget *parent)
    : QWidget(parent)
{
    // setFixedSize(20, 20); // 色块大小
}

void ColorWidget::setColor(const QColor &color)
{
    if (m_color != color) {
        m_color = color;
        update(); // 触发重绘
        emit colorChanged(m_color);
    }
}

void ColorWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);

    // 填充颜色
    p.fillRect(rect(), m_color);
}

void ColorWidget::mousePressEvent(QMouseEvent *)
{
    QColor c = QColorDialog::getColor(m_color, this, "选择颜色");

    if (c.isValid()) {
        setColor(c);
    }
}
