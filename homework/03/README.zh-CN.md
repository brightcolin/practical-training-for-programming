[English](README.md) | [简体中文](README.zh-CN.md)

# 作业 3：循环顺序与程序性能

`CP_Analysis` 实现三维矩阵加法的 IJK、IKJ、JIK、JKI、KIJ、KJI 六种循环顺序。

`CP_AnalysisTest.cpp` 使用边长 64、128、256 的矩阵运行六种算法，打印执行时间与结果元素总和。测试结果可用于比较访问顺序对内存局部性的影响；耗时受机器和编译选项影响。

构建后运行 `build/console/CP_AnalysisTest.exe`，无需输入。
