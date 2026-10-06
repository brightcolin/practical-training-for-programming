#include "widget.h"
#include "tempconverter.h"

#include <QDial>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLCDNumber>
#include <QVBoxLayout>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle("Temperature Converter");

    QGroupBox *celsius_group = new QGroupBox("Celsius", this);
    QDial *celsius_dial = new QDial(celsius_group);
    QLCDNumber *celsius_lcd = new QLCDNumber(celsius_group);

    celsius_dial->setRange(0, 100);
    celsius_lcd->setSegmentStyle(QLCDNumber::Flat);
    celsius_lcd->setDigitCount(3);

    QVBoxLayout *celsius_layout = new QVBoxLayout(celsius_group);
    celsius_layout->addWidget(celsius_lcd);
    celsius_layout->addWidget(celsius_dial);

    QGroupBox *fahrenheit_group = new QGroupBox("Fahrenheit", this);
    QDial *fahrenheit_dial = new QDial(fahrenheit_group);
    QLCDNumber *fahrenheit_lcd = new QLCDNumber(fahrenheit_group);

    fahrenheit_dial->setRange(32, 212);
    fahrenheit_lcd->setSegmentStyle(QLCDNumber::Flat);
    fahrenheit_lcd->setDigitCount(3);

    QVBoxLayout *fahrenheit_layout = new QVBoxLayout(fahrenheit_group);
    fahrenheit_layout->addWidget(fahrenheit_lcd);
    fahrenheit_layout->addWidget(fahrenheit_dial);

    TempConverter *temp_converter = new TempConverter(0, this);

    connect(celsius_dial, &QDial::valueChanged,
            celsius_lcd, qOverload<int>(&QLCDNumber::display));

    connect(fahrenheit_dial, &QDial::valueChanged,
            fahrenheit_lcd, qOverload<int>(&QLCDNumber::display));

    connect(celsius_dial, &QDial::valueChanged,
            temp_converter, &TempConverter::setTempCelsius);

    connect(fahrenheit_dial, &QDial::valueChanged,
            temp_converter, &TempConverter::setTempFahrenheit);

    connect(temp_converter, &TempConverter::tempCelsiusChanged,
            celsius_dial, &QDial::setValue);

    connect(temp_converter, &TempConverter::tempFahrenheitChanged,
            fahrenheit_dial, &QDial::setValue);

    celsius_dial->setValue(temp_converter->tempCelsius());
    fahrenheit_dial->setValue(temp_converter->tempFahrenheit());

    celsius_lcd->display(temp_converter->tempCelsius());
    fahrenheit_lcd->display(temp_converter->tempFahrenheit());

    QHBoxLayout *main_layout = new QHBoxLayout();
    main_layout->addWidget(celsius_group);
    main_layout->addWidget(fahrenheit_group);

    setLayout(main_layout);
    resize(420, 240);
}

Widget::~Widget() = default;
