#include "CP_Analysis.h"

using namespace std;

CP_Analysis::CP_Analysis(int n)
{
    N=n;
}

int CP_Analysis::getN() const
{
    return N;
}

void CP_Analysis::initializeMatrices(vector<vector<vector<int>>>& a,
                               vector<vector<vector<int>>>& b,
                               vector<vector<vector<int>>>& c) const
{
    a.resize(N, vector<vector<int>>(N, vector<int>(N)));
    b.resize(N, vector<vector<int>>(N, vector<int>(N)));
    c.resize(N, vector<vector<int>>(N, vector<int>(N)));

    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < N; j++)
        {
            for (int k = 0; k < N; k++)
            {
                a[i][j][k] = i+j+k;
                b[i][j][k] = i*j+k;
                c[i][j][k] = 0;
            }
        }
    }
}
void CP_Analysis::addMatricesIJK(const vector<vector<vector<int>>>& a,
                               const vector<vector<vector<int>>>& b,
                               vector<vector<vector<int>>>& c) const
{
    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < N; j++)
        {
            for (int k = 0; k < N; k++)
            {
                c[i][j][k] = a[i][j][k] + b[i][j][k];
            }
        }
    }
}
void CP_Analysis::addMatricesIKJ(const vector<vector<vector<int>>>& a,
                               const vector<vector<vector<int>>>& b,
                               vector<vector<vector<int>>>& c) const
{
    for (int i = 0; i < N; i++)
    {
        for (int k = 0; k < N; k++)
        {
            for (int j = 0; j < N; j++)
            {
                c[i][j][k] = a[i][j][k] + b[i][j][k];
            }
        }
    }
}

void CP_Analysis::addMatricesJIK(const vector<vector<vector<int>>>& a,
                               const vector<vector<vector<int>>>& b,
                               vector<vector<vector<int>>>& c) const
{
    for (int j = 0; j < N; j++)
    {
        for (int i = 0; i < N; i++)
        {
            for (int k = 0; k < N; k++)
            {
                c[i][j][k] = a[i][j][k] + b[i][j][k];
            }
        }
    }
}

void CP_Analysis::addMatricesJKI(const vector<vector<vector<int>>>& a,
                               const vector<vector<vector<int>>>& b,
                               vector<vector<vector<int>>>& c) const
{
    for (int j = 0; j < N; j++)
    {
        for (int k = 0; k < N; k++)
        {
            for (int i = 0; i < N; i++)
            {
                c[i][j][k] = a[i][j][k] + b[i][j][k];
            }
        }
    }
}

void CP_Analysis::addMatricesKIJ(const vector<vector<vector<int>>>& a,
                               const vector<vector<vector<int>>>& b,
                               vector<vector<vector<int>>>& c) const
{
    for (int k = 0; k < N; k++)
    {
        for (int i = 0; i < N; i++)
        {
            for (int j = 0; j < N; j++)
            {
                c[i][j][k] = a[i][j][k] + b[i][j][k];
            }
        }
    }
}

void CP_Analysis::addMatricesKJI(const vector<vector<vector<int>>>& a,
                               const vector<vector<vector<int>>>& b,
                               vector<vector<vector<int>>>& c) const
{
    for (int k = 0; k < N; k++)
    {
        for (int j = 0; j < N; j++)
        {
            for (int i = 0; i < N; i++)
            {
                c[i][j][k] = a[i][j][k] + b[i][j][k];
            }
        }
    }
}
