[English](README.md) | [简体中文](README.zh-CN.md)

# Audio Resources

The original submission used 18 audio files selected from Pixabay and converted to WAV. They are excluded from this public archive; the original course folder and local archive copy retain them.

Missing audio makes the corresponding sounds silent. Source code and artwork remain available. The qmake project's DISTFILES list retains the original filenames; it is not currently intended to create a source distribution containing those audio files.

## Restore audio from your original copy

1. Copy the 18 `.wav` files from your original project's `src/assets/sfx/` into this directory.
2. After building, copy the entire `assets/` directory beside the executable.
3. Before including audio in the public repository, record each asset's name, original download URL, creator, license information, and modifications; check the distribution conditions, then adjust the WAV exclusion rule in the root `.gitignore`.

Original filenames and uses are listed in the [asset notes](../../../doc/素材来源.md).
