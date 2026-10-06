#include "CP_Analysis.h"

#include <iostream>
#include <vector>
#include <ctime>

using namespace std;
typedef vector<vector<vector<int>>> Matrix3D;

long long checkResult(const Matrix3D& c, int N)
{
    long long sum = 0;

    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < N; j++)
        {
            for (int k = 0; k < N; k++)
            {
                sum += c[i][j][k];
            }
        }
    }
    return sum;
}

void runOneOrder(const CP_Analysis& analysis, Matrix3D& a, Matrix3D& b, Matrix3D& c, int order)
{
    const char* orderName[6]= {"IJK", "IKJ", "JIK", "JKI", "KIJ", "KJI"};

    clock_t start = clock();

    if (order == 0)
    {
        analysis.addMatricesIJK(a, b, c);
    }
    else if (order == 1)
    {
        analysis.addMatricesIKJ(a, b, c);
    }
    else if (order == 2)
    {
        analysis.addMatricesJIK(a, b, c);
    }
    else if (order == 3)
    {
        analysis.addMatricesJKI(a, b, c);
    }
    else if (order == 4)
    {
        analysis.addMatricesKIJ(a, b, c);
    }
    else if (order == 5)
    {
        analysis.addMatricesKJI(a, b, c);
    }

    clock_t end = clock();
    double time_taken = double(end - start) / CLOCKS_PER_SEC;
    cout << "Time taken for " << orderName[order] << ": " << time_taken << " seconds" << endl;

    long long sum = checkResult(c, analysis.getN());
    cout << "Sum of elements in result matrix: " << sum << endl;
}
void testOneSize(const int N)
{
    
    CP_Analysis analysis(N);
    cout << "Testing matrix addition for size: " << N << endl;
    
    Matrix3D a;
    Matrix3D b;
    Matrix3D c;

    analysis.initializeMatrices(a, b, c);

    for (int order = 0; order < 6; order++)
    {
        runOneOrder(analysis, a, b, c, order);
    }
    cout << endl;
}

int main()
{
    testOneSize(64);
    testOneSize(128);
    testOneSize(256);

    return 0;
}