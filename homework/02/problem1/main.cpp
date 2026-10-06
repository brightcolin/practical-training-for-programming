#include "TwentyfourSolver.h"

#include <iostream>
#include <string>
#include <vector>
#include <set>

int main()
{
    std::vector<int> numbers(4);

    for (size_t i = 0; i < 4; ++i)
    {
        std::cin >> numbers[i];
    }
    
    TwentyfourSolver solver;
    std::set<std::string> solutions = solver.solve(numbers);

    if (solutions.empty())
    {
        std::cout << "no" << std::endl;
    }
    else
    {
        
        for (const auto& solution : solutions)
        {
            std::cout << solution << std::endl;
        }
    }

    return 0;
}