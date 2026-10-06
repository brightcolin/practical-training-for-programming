#include "TwentyfourSolver.h"

#include <string>
#include <cmath>
#include <vector>

const double EPSILON = 1e-6;

std::set<std::string> TwentyfourSolver::solve(const std::vector<int>& numbers)
{
    answers.clear();

    std::vector<Expression> expressions;
    for (size_t i = 0; i < numbers.size(); ++i)
    {
        expressions.push_back(Expression(static_cast<double>(numbers[i]), std::to_string(numbers[i])));
    }
    search(expressions);
    return answers;
}
bool TwentyfourSolver::isEqualToTwentyfour(double value)
{
    return std::fabs(value - 24.0) < EPSILON;
}

void TwentyfourSolver::search(const std::vector<Expression>& expressions)
{
    if (expressions.size() == 1)
    {
        if (isEqualToTwentyfour(expressions[0].getValue()))
        {
            answers.insert(expressions[0].getText());
        }
        return;
    }

    size_t size = expressions.size();
    for (size_t i = 0; i < size; ++i)
    {
        for (size_t j = 0; j < size; ++j)
        {
            if (i == j) continue;

            std::vector<Expression> nextExpressions;
            for (size_t k = 0; k < size; ++k)
            {
                if (k != i && k != j)
                {
                    nextExpressions.push_back(expressions[k]);
                }
            }

            double a = expressions[i].getValue();
            double b = expressions[j].getValue();
            std::string textA = expressions[i].getText();
            std::string textB = expressions[j].getText();

            // Addition
            nextExpressions.push_back(Expression(a + b, "(" + textA + "+" + textB + ")"));
            search(nextExpressions);
            nextExpressions.pop_back();

            // Subtraction
            nextExpressions.push_back(Expression(a - b, "(" + textA + "-" + textB + ")"));
            search(nextExpressions);
            nextExpressions.pop_back();

            // Multiplication
            nextExpressions.push_back(Expression(a * b, "(" + textA + "*" + textB + ")"));
            search(nextExpressions);
            nextExpressions.pop_back();

            // Division
            if (std::fabs(b) > EPSILON) // Avoid division by zero
            {
                nextExpressions.push_back(Expression(a / b, "(" + textA + "/" + textB + ")"));
                search(nextExpressions);
                nextExpressions.pop_back();
            }
        }
    }
}


