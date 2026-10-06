[English](README.md) | [简体中文](README.zh-CN.md)

# Audio Resources

The sound manager loads WAV resources from `assets/sfx/` beside the executable. Missing files make their corresponding sound effects or music silent.

## Resource groups

- Interface: clicks, terminal startup, dialogue, and error feedback.
- Combat: deployment, upgrades, skills, bosses, and core damage.
- Results: victory, failure, and codex unlocks.
- Music: menu, battle, story, and boss tracks.

Expected filenames and uses are listed in the [asset notes](../../../doc/素材来源.md) and the qmake project's DISTFILES list.

## Deployment

Place WAV files in this directory and copy the entire `assets/` directory beside the executable after building. Use audio resources according to their respective licenses.
