#include "TargetSolver.h"

#include <iostream>
#include <vector>

int main()
{
    int n;
    unsigned long long target;
    std::cin >> n >> target;

    std::vector<unsigned long long> numbers(n);
    for (int i = 0; i < n; ++i)
    {
        std::cin >> numbers[i];
    }

    TargetSolver solver(numbers, target);
    TargetResult result = solver.getResult();

    if (result.isFound())
    {
        std::cout << result.getExpression() << std::endl;
    }
    else
    {
        std::cout << "No" << std::endl;
        if (result.hasGreaterValue())
        {
            std::cout << result.getGreaterValue() << std::endl;
        }
        else
        {
            std::cout << -1 << std::endl;
        }
    }

    return 0;
}