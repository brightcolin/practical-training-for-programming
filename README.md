[English](README.md) | [简体中文](README.zh-CN.md)

# Practical Training For Programming

A source archive of five homework assignments and the final project, **Time Repair Bureau**. Original coursework code is preserved; navigation and build instructions are added separately.

## Assignments

| Assignment | Topics | Directory |
| --- | --- | --- |
| 1 | Object-oriented programming, polymorphism, templates, operator overloading | [Homework 01](homework/01/) |
| 2 | 24-point expressions and target-value expressions | [Homework 02](homework/02/) |
| 3 | Six loop orders for 3D matrix addition and performance comparison | [Homework 03](homework/03/) |
| 4 | Qt numeric keypad and temperature conversion | [Homework 04](homework/04/) |
| 5 | Qt event filtering and Enter-key input handling | [Homework 05](homework/05/) |
| Final project | C++17 / Qt Widgets tower defense game | [Time Repair Bureau](final-project/time-repair-bureau/) |

## What is scripts/?

`scripts/` contains a build helper added during archiving, separate from the original assignment code.

`scripts/build-console.ps1` is a Windows PowerShell script. It invokes GCC / MinGW to compile the six console programs from assignments 1-3 into `build/console/`. It runs only when invoked manually, does not compile the Qt projects, and does not rewrite source files.

Run from the repository root:

```powershell
./scripts/build-console.ps1
```

If `g++` is not on PATH, specify the actual compiler path on your computer:

```powershell
./scripts/build-console.ps1 -Compiler 'D:/msys64/ucrt64/bin/g++.exe'
```

To run the generated programs, make sure the compiler's runtime-library directory is on PATH.

## Qt projects

Assignments 4-5 require Qt 6.5 or later and CMake 3.19 or later. Open each project's `CMakeLists.txt` in Qt Creator and select a Desktop Kit.

The final project uses `final-project/time-repair-bureau/src/TimeRepairBureau.pro`. See the [build and run instructions](final-project/time-repair-bureau/doc/编译运行说明.md).

## Archive scope and checks

- Source code, Qt project files, game configuration, and artwork are included. Original coursework code is unchanged.
- Lecture materials, reports, self-evaluation sheets, submission archives, videos, saved progress, build products, and Qt runtime libraries are excluded.
- The public [AI disclosure](final-project/time-repair-bureau/doc/AI工具使用声明.md) omits the student's name and ID. [Asset notes](final-project/time-repair-bureau/doc/素材来源.md) are retained.
- The 18 Pixabay WAV files remain local and are excluded from the public repository. Missing audio makes the corresponding sounds silent; see the [audio notes](final-project/time-repair-bureau/src/assets/sfx/README.md) for restoration.
- All six console programs compiled and passed sample-run checks. Qt projects were not rebuilt during archiving because the original Qt installation is no longer present.
- No repository-wide open-source license has been added. Course templates and third-party assets remain subject to their original terms.
