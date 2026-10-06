[English](README.md) | [简体中文](README.zh-CN.md)

# 程序设计实训 · Practical Training For Programming

本仓库整理课程的 5 次平时作业与最终大作业《时间修补局》。原有代码保持原样，补充目录导航和编译说明。

## 作业导航

| 作业 | 内容 | 路径 |
| --- | --- | --- |
| 1 | 面向对象、多态、函数模板、运算符重载 | [作业 1](homework/01/README.zh-CN.md) |
| 2 | 24 点求解、加法与乘法目标值求解 | [作业 2](homework/02/README.zh-CN.md) |
| 3 | 三维矩阵加法的六种循环顺序与性能比较 | [作业 3](homework/03/README.zh-CN.md) |
| 4 | Qt 数字键盘、摄氏与华氏温度转换 | [作业 4](homework/04/README.zh-CN.md) |
| 5 | Qt 事件过滤器与回车输入处理 | [作业 5](homework/05/README.zh-CN.md) |
| 最终大作业 | C++17 / Qt Widgets 塔防游戏《时间修补局》 | [时间修补局](final-project/time-repair-bureau/README.zh-CN.md) |

## scripts 是什么？

`scripts/` 存放本次整理新增的编译辅助脚本，不属于原课程作业代码。

`scripts/build-console.ps1` 是 Windows PowerShell 脚本，调用 GCC / MinGW 编译前 3 次作业的 6 个控制台程序，将结果放入 `build/console/`。它只在手动执行时运行，不会在浏览 GitHub 时自动执行，也不会编译 Qt 项目或改写源码。

在仓库根目录执行：

```powershell
./scripts/build-console.ps1
```

若 `g++` 不在 PATH 中，可指定本机编译器的实际路径：

```powershell
./scripts/build-console.ps1 -Compiler 'D:/msys64/ucrt64/bin/g++.exe'
```

运行生成的程序时，需将对应编译器的运行库目录加入 PATH。

## Qt 项目

作业 4、5 使用 Qt 6.5 或以上与 CMake 3.19 或以上。使用 Qt Creator 分别打开各项目的 `CMakeLists.txt` 并配置 Desktop Kit。

最终大作业使用 qmake 工程 `final-project/time-repair-bureau/src/TimeRepairBureau.pro`，详见 [编译运行说明](final-project/time-repair-bureau/doc/编译运行说明.md)。

## 归档与验证

- 收录源码、Qt 工程、游戏配置与图片；未修改原有代码。
- 不收录课件、作业报告、自评表、提交压缩包、演示视频、运行存档、编译产物和 Qt 运行库。
- [AI 工具使用声明](final-project/time-repair-bureau/doc/AI工具使用声明.md) 的公开副本去掉姓名和学号；保留 [素材来源说明](final-project/time-repair-bureau/doc/素材来源.md)。
- 18 个 Pixabay WAV 文件只保留本地，未公开上传；缺少音频时对应声音静音，恢复方法见 [音频说明](final-project/time-repair-bureau/src/assets/sfx/README.zh-CN.md)。
- 6 个普通 C++ 程序通过编译及样例运行检查；Qt 项目未在本次归档中重新编译，因为原 Qt 安装路径已不存在。
- 未添加统一开源许可证；课程模板和第三方素材按各自原始条件使用。
