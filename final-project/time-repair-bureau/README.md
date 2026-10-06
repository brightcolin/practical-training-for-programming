[English](README.md) | [简体中文](README.zh-CN.md)

# Time Repair Bureau

A story-driven 2D grid-based tower defense game built with C++17 and Qt Widgets. Deploy defensive devices across damaged timelines and protect the time core from anomalies.

## Features

- Six story levels with distinct bosses.
- Six tower types, six enemy types, terrain effects, resources, upgrades, and active skills.
- Story communications, paused tutorials, an archive, and a codex.
- JSON configuration and persistent progress and settings.
- A custom level editor with save, load, and playtest support.
- Three reverse-simulation levels: arrange enemies while the system deploys defensive towers.
- AI-assisted artwork and optional WAV sound effects and music.

## Build and run

Open `src/TimeRepairBureau.pro` in Qt Creator and select a Qt 6 Desktop Kit with C++17 support. The project targets Windows; Qt 6.5.3 MinGW 64-bit is the reference configuration.

After building, place `src/assets/` and `src/config/` beside the executable as `assets/` and `config/`. See the [build and run guide](doc/编译运行说明.md) for command-line instructions.

WAV resources can be added to `src/assets/sfx/`; see the [audio resource guide](src/assets/sfx/README.md). Missing audio is handled silently.

## Project structure

- `src/`: application code and the qmake project.
- `src/config/`: tower configuration.
- `src/assets/ai/`: backgrounds, sprites, and character artwork.
- `src/assets/sfx/`: audio resource directory.
- `doc/`: build instructions and development documentation.

## Development and assets

See the [AI development disclosure](doc/AI工具使用声明.md) and [asset notes](doc/素材来源.md).
