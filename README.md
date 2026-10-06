# Practical Training For Programming · 程序设计实训

本仓库整理课程的 5 次平时作业与最终大作业《时间修补局》。原始作业源码与收录的图片、配置保持原样，补充目录导航、构建脚本和说明。

## 作业导航

| 作业 | 内容 | 路径 |
| --- | --- | --- |
| 1 | 面向对象、多态、函数模板、运算符重载 | [homework/01](homework/01/) |
| 2 | 24 点求解、加法与乘法目标值求解 | [homework/02](homework/02/) |
| 3 | 三维矩阵加法的六种循环顺序与性能比较 | [homework/03](homework/03/) |
| 4 | Qt 数字键盘、摄氏与华氏温度转换 | [homework/04](homework/04/) |
| 5 | Qt 事件过滤器与回车输入处理 | [homework/05](homework/05/) |
| 最终大作业 | C++17 / Qt Widgets 塔防游戏《时间修补局》 | [final-project/time-repair-bureau](final-project/time-repair-bureau/) |

## 编译普通 C++ 作业

需要支持 C++17 的 GCC / MinGW。Windows PowerShell 在仓库根目录执行：

```powershell
./scripts/build-console.ps1
```

如果 `g++` 不在 PATH 中，可指定编译器：

```powershell
./scripts/build-console.ps1 -Compiler 'D:/msys64/ucrt64/bin/g++.exe'
```

生成的 6 个程序位于 `build/console/`。运行时，请确保编译器对应的运行库目录在 PATH 中。

## 编译 Qt 作业

作业 4、5 使用 Qt 6.5 或以上及 CMake 3.19 或以上。使用 Qt Creator 分别打开各项目的 `CMakeLists.txt`，配置 Desktop Kit 后构建。

最终大作业保留 qmake 工程，详见其 [编译运行说明](final-project/time-repair-bureau/doc/编译运行说明.md)。

## 归档范围与声明

- 收录作业源码、Qt 工程、游戏配置和图片。
- 课件、作业报告、自评表、提交压缩包、演示视频、编译缓存、可执行文件和 Qt 运行库不收录。
- 整理版未改变原始 `.cpp`、`.h`、`.ui`、`.pro` 与已有 CMake 工程；大作业的三个展开副本经 SHA-256 比较完全一致。
- 保留大作业的 [AI 工具使用声明](final-project/time-repair-bureau/doc/AI工具使用声明.md) 和 [素材来源说明](final-project/time-repair-bureau/doc/素材来源.md)。声明副本省略姓名与学号。
- 原作业音频来自 Pixabay。公开仓库暂不收录 18 个 WAV 文件，因为现有资料未列出逐项下载链接与授权记录；这些文件保留在原课程目录和本地整理副本。游戏缺少音频时对应声音会静音，恢复说明见 [assets/sfx/README.md](final-project/time-repair-bureau/src/assets/sfx/README.md)。
- 本仓库未添加统一开源许可证。课程模板及第三方素材的权利以各自原始条件为准。

本地归档验证结果见 [docs/验证记录.md](docs/验证记录.md)。
