#include "widget.h"

#include <QVBoxLayout>
#include <QGridLayout>
#include <QPushButton>

Widget::Widget(QWidget *parent)
    : QWidget(parent),
    display(new QLineEdit(this)),
    signal_mapper(new QSignalMapper(this))
{
    setWindowTitle("Qt Keypad");

    display->setReadOnly(true);
    display->setMinimumHeight(40);
    display->setPlaceholderText("Press number buttons...");
    display->setAlignment(Qt::AlignRight);

    QGridLayout *keypad_layout = new QGridLayout();

    for (int digit = 1; digit <= 9; ++digit)
    {
        QPushButton *button = new QPushButton(QString::number(digit), this);
        button->setMaximumSize(60, 45);

        connect(button, &QPushButton::clicked,
                signal_mapper, qOverload<>(&QSignalMapper::map));

        signal_mapper->setMapping(button, digit);

        int row = (digit - 1) / 3;
        int column = (digit - 1) % 3;

        keypad_layout->addWidget(button, row, column);
    }

    QPushButton *zero_button = new QPushButton("0", this);
    zero_button->setMaximumSize(60, 45);

    connect(zero_button, &QPushButton::clicked,
            signal_mapper, qOverload<>(&QSignalMapper::map));

    signal_mapper->setMapping(zero_button, 0);
    keypad_layout->addWidget(zero_button, 3, 1);

    connect(signal_mapper, &QSignalMapper::mappedInt,
            this, &Widget::AppendDigit);

    QPushButton *clear_button = new QPushButton("Clear", this);
    clear_button->setMinimumHeight(40);

    connect(clear_button, &QPushButton::clicked,
            this, &Widget::ClearDisplay);

    QVBoxLayout *main_layout = new QVBoxLayout(this);
    main_layout->addWidget(display);
    main_layout->addLayout(keypad_layout);
    main_layout->addWidget(clear_button);
    setLayout(main_layout);

    resize(260,300);

}

Widget::~Widget()
{
}

void Widget::AppendDigit(int digit)
{
    display->setText(display->text() + QString::number(digit));
}

void Widget::ClearDisplay()
{
    display->clear();
}
