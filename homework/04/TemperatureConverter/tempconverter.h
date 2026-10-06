#ifndef TEMPCONVERTER_H
#define TEMPCONVERTER_H

#include <QObject>

class TempConverter : public QObject
{
    Q_OBJECT
public:
    explicit TempConverter(int temp_celsius=0, QObject *parent = nullptr);

    int tempCelsius() const;
    int tempFahrenheit() const;

public slots:
    void setTempCelsius(int temp_celsius);
    void setTempFahrenheit(int temp_fahrenheit);

signals:
    void tempCelsiusChanged(int temp_celsius);
    void tempFahrenheitChanged(int temp_fahrenheit);

private:
    int temp_celsius;
};

#endif // TEMPCONVERTER_H
