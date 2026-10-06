#ifndef TWENTYFOURSOLVER_H
#define TWENTYFOURSOLVER_H

#include "Expression.h"

#include <vector>
#include <string>
#include <set>

class TwentyfourSolver
{
public:
    std::set<std::string> solve(const std::vector<int>& numbers);

private:
    void search(const std::vector<Expression>& expressions);

    static bool isEqualToTwentyfour(double value);

    std::set<std::string> answers;
};

#endif // TWENTYFOURSOLVER_H