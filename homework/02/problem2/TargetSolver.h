#ifndef TARGETSOLVER_H
#define TARGETSOLVER_H

#include <string>
#include <vector>

class TargetResult
{
public:
    TargetResult();
    TargetResult(bool found, const std::string& expression,
                bool hasGreaterValue, unsigned long long greaterValue);
    
    bool isFound() const;
    std::string getExpression() const;
    bool hasGreaterValue() const;
    unsigned long long getGreaterValue() const;

private:
    bool m_found;
    std::string m_expression;
    bool m_hasGreaterValue;
    unsigned long long m_greaterValue;
};

class TargetSolver
{
public:
    TargetSolver(const std::vector<unsigned long long>& numbers,
                unsigned long long target);
    
    TargetResult getResult() const;

private:
    TargetResult m_result;
};

#endif // TARGETSOLVER_H
