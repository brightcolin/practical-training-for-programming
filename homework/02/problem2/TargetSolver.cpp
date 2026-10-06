#include "TargetSolver.h"

#include <string>
#include <map>

TargetResult::TargetResult()
    : m_found(false), m_expression(""), m_hasGreaterValue(false), m_greaterValue(0) {}

TargetResult::TargetResult(bool found, const std::string& expression,
                            bool hasGreaterValue, unsigned long long greaterValue)
    : m_found(found), m_expression(expression), m_hasGreaterValue(hasGreaterValue), m_greaterValue(greaterValue) {}

bool TargetResult::isFound() const
{
    return m_found;
}

std::string TargetResult::getExpression() const
{
    return m_expression;
}

bool TargetResult::hasGreaterValue() const
{
    return m_hasGreaterValue;
}

unsigned long long TargetResult::getGreaterValue() const
{
    return m_greaterValue;
}

TargetResult TargetSolver::getResult() const
{
    return m_result;
}

TargetSolver::TargetSolver(const std::vector<unsigned long long>& numbers,
                             unsigned long long target)
{
    std::map<unsigned long long, std::string> states;

    states[numbers[0]] = std::to_string(numbers[0]);

    for (size_t i = 1; i < numbers.size(); ++i)
    {
        std::map<unsigned long long, std::string> newStates;

        for (const auto& state : states)
        {
            unsigned long long currentValue = state.first;
            const std::string& currentExpression = state.second;

            unsigned long long newValue = currentValue + numbers[i];
            std::string newExpression = currentExpression + "+" + std::to_string(numbers[i]);
            newStates[newValue] = newExpression;

            newValue = currentValue * numbers[i];
            newExpression = currentExpression + "*" + std::to_string(numbers[i]);
            newStates[newValue] = newExpression;
        }

        states = newStates;
    }

    auto targetIterator = states.find(target);
    if (targetIterator != states.end())
    {
        m_result = TargetResult(true, targetIterator->second, false, 0);
    }
    else
    {
        unsigned long long closestGreaterValue = 0;

        auto closestIterator = states.upper_bound(target);

        if (closestIterator != states.end())
        {
            closestGreaterValue = closestIterator->first;
            m_result = TargetResult(false, "", true, closestGreaterValue);
        }
        else
        {
            m_result = TargetResult(false, "", false, 0);
        }
    }
}