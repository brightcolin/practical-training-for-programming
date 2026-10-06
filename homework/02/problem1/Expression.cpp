#include "Expression.h"

Expression::Expression(double value, const std::string& text)
    : value(value), text(text) {}

double Expression::getValue() const
{
    return value;
}

std::string Expression::getText() const
{
    return text;
}
