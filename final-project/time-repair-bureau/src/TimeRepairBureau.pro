QT += core gui widgets

CONFIG += c++17
CONFIG -= app_bundle

TEMPLATE = app
TARGET = TimeRepairBureau

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    gamewidget.cpp \
    editorwidget.cpp \
    levelmanager.cpp \
    progressmanager.cpp \
    entityfactory.cpp \
    entityrules.cpp \
    soundmanager.cpp

HEADERS += \
    mainwindow.h \
    gamewidget.h \
    editorwidget.h \
    gametypes.h \
    levelmanager.h \
    progressmanager.h \
    entityfactory.h \
    entityrules.h \
    soundmanager.h

win32:LIBS += -lwinmm

DISTFILES += \
    config/towers.json \
    assets/ai/bg-main.png \
    assets/ai/bg-victory.png \
    assets/ai/bg-failure.png \
    assets/ai/bg-archive.png \
    assets/ai/sheet-towers.png \
    assets/ai/sheet-enemies.png \
    assets/ai/sheet-enemies-v2.png \
    assets/ai/sheet-operators.png \
    assets/ai/sheet-tiles.png \
    assets/ai/tower-shooter.png \
    assets/ai/tower-slow.png \
    assets/ai/tower-splash.png \
    assets/ai/tower-laser.png \
    assets/ai/tower-resource.png \
    assets/ai/tower-wall.png \
    assets/ai/enemy-normal.png \
    assets/ai/enemy-fast.png \
    assets/ai/enemy-armored.png \
    assets/ai/enemy-resistant.png \
    assets/ai/enemy-splitter.png \
    assets/ai/enemy-boss.png \
    assets/ai/enemy-normal-v2.png \
    assets/ai/enemy-fast-v2.png \
    assets/ai/enemy-armored-v2.png \
    assets/ai/enemy-resistant-v2.png \
    assets/ai/enemy-splitter-v2.png \
    assets/ai/enemy-boss-v2.png \
    assets/ai/tile-stable.png \
    assets/ai/tile-path.png \
    assets/ai/tile-discount.png \
    assets/ai/tile-anchor.png \
    assets/ai/tile-accelerate.png \
    assets/ai/tile-portal.png \
    assets/sfx/ui-click.wav \
    assets/sfx/terminal-boot.wav \
    assets/sfx/dialogue-blip.wav \
    assets/sfx/mission-start.wav \
    assets/sfx/build.wav \
    assets/sfx/upgrade.wav \
    assets/sfx/freeze.wav \
    assets/sfx/boss-alert.wav \
    assets/sfx/boss-stinger.wav \
    assets/sfx/error-deny.wav \
    assets/sfx/core-damage.wav \
    assets/sfx/codex-unlock.wav \
    assets/sfx/victory.wav \
    assets/sfx/failure.wav \
    assets/sfx/ambience-menu.wav \
    assets/sfx/ambience-battle.wav \
    assets/sfx/story-bgm.wav \
    assets/sfx/boss-bgm.wav
