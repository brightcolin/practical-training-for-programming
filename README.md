[English](README.md) | [简体中文](README.zh-CN.md)

# Practical Training For Programming

A collection of C++ programming exercises and Qt applications, including **Time Repair Bureau**, a 2D tower defense game built with C++17 and Qt Widgets.

## Projects

| Project | Description | Source |
| --- | --- | --- |
| Object-oriented programming | Polymorphism, function templates, and operator overloading | [Homework 01](homework/01/) |
| Expression solvers | The 24-point game and target-value expressions | [Homework 02](homework/02/) |
| Matrix performance | Six traversal orders for 3D matrix addition | [Homework 03](homework/03/) |
| Qt signals and slots | Numeric keypad and temperature converter | [Homework 04](homework/04/) |
| Qt event handling | Event filtering and keyboard input | [Homework 05](homework/05/) |
| Time Repair Bureau | Story-driven tower defense, a level editor, and reverse simulations | [Final project](final-project/time-repair-bureau/) |

## Requirements

- C++17-compatible GCC / MinGW for the console programs.
- Windows PowerShell for the console build command below.
- Qt 6.5 or later and CMake 3.19 or later for the Qt homework projects.
- Qt 6 with qmake for Time Repair Bureau; Windows with a MinGW Desktop Kit is recommended.

## Build

### Console programs

With `g++` on PATH, run from the repository root:

```powershell
./scripts/build-console.ps1
```

The six executables are generated in `build/console/`. The compiler's runtime libraries must be available on PATH when running them. Each project's README describes its input and output.

### Qt applications

Open a project's `CMakeLists.txt` in Qt Creator, select a Desktop Kit, then build and run:

- `homework/04/QtKeypad/CMakeLists.txt`
- `homework/04/TemperatureConverter/CMakeLists.txt`
- `homework/05/EventFilter/CMakeLists.txt`

For Time Repair Bureau, open `final-project/time-repair-bureau/src/TimeRepairBureau.pro`. See its [build and run guide](final-project/time-repair-bureau/doc/编译运行说明.md) for resource deployment.

## Time Repair Bureau

- Six story levels with bosses, defense towers, enemies, terrain, resources, and upgrades.
- Story communications, interactive tutorials, an archive, and a codex.
- JSON configuration, saved progress, and a custom level editor.
- Three reverse-simulation levels with player-arranged enemies.

See the [project README](final-project/time-repair-bureau/), [asset notes](final-project/time-repair-bureau/doc/素材来源.md), and [AI development disclosure](final-project/time-repair-bureau/doc/AI工具使用声明.md).
