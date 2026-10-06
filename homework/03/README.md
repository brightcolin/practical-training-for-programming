[English](README.md) | [简体中文](README.zh-CN.md)

# Homework 3: Loop Order and Performance

`CP_Analysis` implements six loop orders for 3D matrix addition: IJK, IKJ, JIK, JKI, KIJ, and KJI.

`CP_AnalysisTest.cpp` runs each method on matrices with side lengths 64, 128, and 256. It prints elapsed times and the sum of result elements. The timings illustrate the effect of memory access order and depend on hardware and compiler options.

After building, run `build/console/CP_AnalysisTest.exe`. No input is required.
