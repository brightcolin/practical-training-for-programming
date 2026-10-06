#include "widget.h"
#include "ui_widget.h"

#include <QEvent>
#include <QKeyEvent>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);

    ui->lineEditInput->installEventFilter(this);

    ui->lineEditInput->setPlaceholderText("请输入文字，按回车添加");
    ui->textEditOutput->setReadOnly(true);
}

Widget::~Widget()
{
    delete ui;
}

bool Widget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == ui->lineEditInput && event->type() == QEvent::KeyPress)
    {
        QKeyEvent *key_event = static_cast<QKeyEvent *>(event);

        if(key_event->key() == Qt::Key_Return || key_event->key() == Qt::Key_Enter)
        {
            QString text = ui->lineEditInput->text();

            if (!text.isEmpty())
            {
                ui->textEditOutput->append(text);
                ui->lineEditInput->clear();
            }

            return true;
        }
    }

    return QWidget::eventFilter(watched, event);
}