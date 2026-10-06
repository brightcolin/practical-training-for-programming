[English](README.md) | [简体中文](README.zh-CN.md)

# 程序设计实训 · Practical Training For Programming

C++ 编程练习与 Qt 应用项目集合，包含使用 C++17 和 Qt Widgets 开发的二维塔防游戏《时间修补局》。

## 项目预览

![时间修补局战斗界面](final-project/time-repair-bureau/doc/screenshots/gameplay.png)

《时间修补局》结合网格塔防、地形效果、升级系统与自定义关卡编辑器。更多画面见 [游戏项目展示](final-project/time-repair-bureau/README.zh-CN.md#游戏展示)。

## 项目

| 项目 | 内容 | 源码 |
| --- | --- | --- |
| 面向对象编程 | 多态、函数模板、运算符重载 | [作业 1](homework/01/README.zh-CN.md) |
| 表达式求解 | 24 点与目标值表达式 | [作业 2](homework/02/README.zh-CN.md) |
| 矩阵性能分析 | 三维矩阵加法的六种遍历顺序 | [作业 3](homework/03/README.zh-CN.md) |
| Qt 信号与槽 | 数字键盘与温度转换器 | [作业 4](homework/04/README.zh-CN.md) |
| Qt 事件处理 | 事件过滤与键盘输入 | [作业 5](homework/05/README.zh-CN.md) |
| 时间修补局 | 剧情塔防、关卡编辑器与逆向推演 | [最终项目](final-project/time-repair-bureau/README.zh-CN.md) |

## 环境要求

- 控制台程序：支持 C++17 的 GCC / MinGW。
- 以下控制台构建命令使用 Windows PowerShell。
- Qt 平时作业：Qt 6.5 或以上、CMake 3.19 或以上。
- 时间修补局：Qt 6 与 qmake，推荐 Windows 和 MinGW Desktop Kit。

## 构建

### 控制台程序

将 `g++` 加入 PATH，在仓库根目录执行：

```powershell
./scripts/build-console.ps1
```

6 个可执行文件生成在 `build/console/`。运行时需确保编译器对应的运行库目录位于 PATH。各项目 README 中说明了输入和输出方式。

### Qt 应用

在 Qt Creator 中打开项目的 `CMakeLists.txt`，选择 Desktop Kit 后构建并运行：

- `homework/04/QtKeypad/CMakeLists.txt`
- `homework/04/TemperatureConverter/CMakeLists.txt`
- `homework/05/EventFilter/CMakeLists.txt`

时间修补局使用 `final-project/time-repair-bureau/src/TimeRepairBureau.pro`，资源部署方法见 [编译运行说明](final-project/time-repair-bureau/doc/编译运行说明.md)。

## 时间修补局

- 六个剧情关卡，包含 Boss、防御塔、敌人、地形、资源与升级系统。
- 剧情通讯、交互教学、档案与图鉴。
- JSON 数值配置、进度保存和自定义关卡编辑器。
- 三个由玩家编排敌人的逆向推演关卡。

详见 [项目介绍](final-project/time-repair-bureau/README.zh-CN.md)、[素材说明](final-project/time-repair-bureau/doc/素材来源.md) 与 [AI 开发声明](final-project/time-repair-bureau/doc/AI工具使用声明.md)。
