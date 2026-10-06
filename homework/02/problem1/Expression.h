#ifndef EXPRESSION_H
#define EXPRESSION_H

#include <string>

class Expression
{
public:
   Expression(double value, const std::string& text);

    double getValue() const;
    std::string getText() const;

private:
    double value;
    std::string text;
};

#endif // EXPRESSION_H
