[English](README.md) | [简体中文](README.zh-CN.md)

# 时间修补局 · Time Repair Bureau

课程最终大作业：使用 C++17 与 Qt Widgets 实现的二维网格塔防游戏。

## 主要功能

- 六个主线关卡、防御塔与敌人类型、地形、Boss、资源和升级系统。
- 剧情通讯、暂停式教学、档案图鉴与设置页面。
- JSON 数值配置、进度保存、自定义关卡编辑器。
- 三个逆向推演关卡，玩家编排敌人，由系统自动部署防御塔。
- AI 辅助生成的视觉素材，以及支持 WAV 音效与音乐的声音管理模块。

## 目录

- `src/`：源码、qmake 工程、`config/` 配置和 `assets/ai/` 图片。
- `doc/`：构建说明、素材来源和 AI 工具使用声明。

在 Qt Creator 中打开 `src/TimeRepairBureau.pro`。原项目使用 Qt 6.5.3 MinGW 64-bit；当前归档没有附带编译好的程序与 Qt DLL。完整构建与资源部署方法见 [编译运行说明](doc/编译运行说明.md)。

公开仓库暂未收录原提交版本的 18 个 Pixabay 音频文件。缺少音频时程序对应声音会静音，恢复方法见 [音频说明](src/assets/sfx/README.md)。原 qmake 工程的音频清单保留，用于说明原版资源组成。

作者与素材声明见 [AI 工具使用声明](doc/AI工具使用声明.md) 和 [素材来源](doc/素材来源.md)。
