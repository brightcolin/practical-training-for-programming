[English](README.md) | [简体中文](README.zh-CN.md)

# Time Repair Bureau

The course's final project: a 2D grid-based tower defense game built with C++17 and Qt Widgets.

## Features

- Six story levels, defense towers, enemy types, terrain, bosses, resources, and upgrades.
- Story communications, paused tutorials, an archive, a codex, and settings.
- JSON configuration, saved progress, and a custom level editor.
- Three reverse-simulation levels where the player arranges enemies and the system places defense towers.
- AI-assisted artwork and a sound manager with WAV sound-effect and music support.

## Layout and building

- `src/`: source files, the qmake project, `config/`, and artwork in `assets/ai/`.
- `doc/`: build instructions, asset notes, and AI disclosure.

Open `src/TimeRepairBureau.pro` in Qt Creator. The original project used Qt 6.5.3 MinGW 64-bit. Compiled executables and Qt DLLs are not included. See the [build and run instructions](doc/编译运行说明.md) for building and resource deployment.

The original submission's 18 Pixabay audio files are excluded from the public archive. Missing audio makes corresponding sounds silent; see the [audio notes](src/assets/sfx/README.md). The qmake file retains the original audio filename list.

See the [AI disclosure](doc/AI工具使用声明.md) and [asset notes](doc/素材来源.md) for development and resource information.
