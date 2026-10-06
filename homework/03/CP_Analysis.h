#ifndef CP_ANALYSIS_H
#define CP_ANALYSIS_H

#include <vector>

class CP_Analysis
{
public:

    CP_Analysis(int n);

    int getN() const;

    void initializeMatrices(std::vector<std::vector<std::vector<int>>>& a,
                            std::vector<std::vector<std::vector<int>>>& b,
                            std::vector<std::vector<std::vector<int>>>& c) const;

    void addMatricesIJK(const std::vector<std::vector<std::vector<int>>>& a,
                            const std::vector<std::vector<std::vector<int>>>& b,
                            std::vector<std::vector<std::vector<int>>>& c) const;

    void addMatricesIKJ(const std::vector<std::vector<std::vector<int>>>& a,
                            const std::vector<std::vector<std::vector<int>>>& b,
                            std::vector<std::vector<std::vector<int>>>& c) const;
    
    void addMatricesJIK(const std::vector<std::vector<std::vector<int>>>& a,
                            const std::vector<std::vector<std::vector<int>>>& b,
                            std::vector<std::vector<std::vector<int>>>& c) const;

    void addMatricesJKI(const std::vector<std::vector<std::vector<int>>>& a,
                            const std::vector<std::vector<std::vector<int>>>& b,
                            std::vector<std::vector<std::vector<int>>>& c) const;
    
    void addMatricesKIJ(const std::vector<std::vector<std::vector<int>>>& a,
                            const std::vector<std::vector<std::vector<int>>>& b,
                            std::vector<std::vector<std::vector<int>>>& c) const;

    void addMatricesKJI(const std::vector<std::vector<std::vector<int>>>& a,
                            const std::vector<std::vector<std::vector<int>>>& b,
                            std::vector<std::vector<std::vector<int>>>& c) const;
                            
private:
    int N;
};

#endif // CP_ANALYSIS_H
