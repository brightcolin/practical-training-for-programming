#include "tempconverter.h"

TempConverter::TempConverter(int temp_celsius, QObject *parent)
    : QObject(parent),
    temp_celsius(temp_celsius)
{
}

int TempConverter::tempCelsius() const
{
    return temp_celsius;
}

int TempConverter::tempFahrenheit() const
{
    return temp_celsius * 9 / 5 + 32;
}

void TempConverter::setTempCelsius(int new_temp_celsius)
{
    if (temp_celsius == new_temp_celsius)
    {
        return;
    }

    temp_celsius = new_temp_celsius;

    emit tempCelsiusChanged(tempCelsius());
    emit tempFahrenheitChanged(tempFahrenheit());
}

void TempConverter::setTempFahrenheit(int temp_fahrenheit)
{
    int new_temp_celsius = (temp_fahrenheit - 32) * 5 / 9;
    setTempCelsius(new_temp_celsius);
}