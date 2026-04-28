# Nexus Mods page draft

## Mod name

Crimson Desert AutoLoot CN

## Short description

Open-source Chinese AutoLoot ASI plugin for Crimson Desert with ground loot, corpse interaction, item filtering, and a Chinese configuration UI.

## Main description

Crimson Desert AutoLoot CN is an unofficial Windows x64 ASI plugin for Crimson Desert.

It adds automatic ground-loot interaction, automatic corpse/search interaction, category-based item filtering, per-item block lists, configurable hotkeys, and a Chinese configuration UI.

This project is open source. Source code, releases, and issue tracking are available on GitHub:

https://github.com/myduoy/Crimson-Desert-AutoLoot-CN

## Important language notice

This release is primarily maintained in Simplified Chinese.

If you want to use another language, you need to translate the plugin text/configuration files yourself. The mod includes editable configuration and item-list files, so translations can be made by editing the included text data. Untranslated item names may fall back to the original game/internal text.

中文说明：本补丁默认以简体中文维护。如果需要其他语言，请用户自行翻译随包提供的配置文本和物品表文本。

## Features

- Automatic ground-loot pickup by detecting interact prompts and sending the configured interact key.
- Automatic corpse/search interaction.
- Item filtering by category.
- Per-item allow/block control.
- Searchable item list in the configuration UI.
- Configurable hotkeys for plugin toggle and panel toggle.
- Hot-reloadable configuration.
- Lower-stutter filtering path in v0.1.1: expensive ground-item text resolution is moved to the plugin worker thread instead of blocking the game hook callback.

## Current limitations

- Combat-state pause / weapon-drawn detection is not included in this public build. Earlier probing code was removed for stability.
- This is an ASI plugin and requires a working ASI loader setup for Crimson Desert.
- Filtering depends on recognized interaction/item text. Items that cannot be resolved may fall back to safer behavior depending on your filter settings.
- The plugin is unofficial and not affiliated with Pearl Abyss or Crimson Desert.

## Installation

1. Close the game.
2. Extract the archive.
3. Copy `crimson_autoloot_cn.asi` into:

   ```text
   <Crimson Desert>\bin64\
   ```

4. Copy the included support folder into the same directory:

   ```text
   <Crimson Desert>\bin64\crimson_autoloot_cn\
   ```

5. Start the game.

Default keys:

```text
F9  Toggle AutoLoot
F10 Open/close configuration panel
E   Interact key, should match the in-game basic interaction key
```

## Configuration

Run `crimson_autoloot_config.exe` from the support folder, or press `F10` in game if the in-game panel is enabled.

Main settings:

- `Enable`: master switch.
- `Ground`: automatic ground-loot interaction.
- `Corpse`: automatic corpse/search interaction.
- `Filter`: item filter switch.
- `Only when game is foreground`: do not send keys when the game window is not focused.
- `Interact key`: default `E`.
- `Toggle key`: default `F9`.
- `Panel key`: default `F10`.

Filtering behavior:

- Left-side category checkbox controls whether a whole category is allowed.
- Turning off a category disables pickup for all items in that category.
- Turning off a single item in the right-side list adds that item to the block list.
- Search only filters the current category view.

## Changelog

### v0.1.1

- Improved item filtering reliability by requiring confirmed item-name resolution when filters are enabled.
- Added candidate-string resolution before broad memory text scanning.
- Moved expensive ground-item text resolution out of the game hook callback and into the plugin worker thread.
- Reduced repeated text scans for cached ground-item results.
- Fixed `DebugLog=0` so runtime logging can be disabled completely.
- Kept combat-pause / weapon-state probing disabled for stability.

### v0.1.0

- Added Chinese AutoLoot ASI plugin.
- Added Chinese configuration UI.
- Added ground loot and corpse interaction automation.
- Added category-based item filter and per-item block list.
- Added hotkeys for plugin toggle and config panel.
- Removed combat-pause and weapon-state probing logic from the public build.

## Permissions and credits

License: MIT.

This package does not include any Crimson Desert game files.

No files from the original third-party AutoLoot plugin are included in this package.

## Support

Please report bugs on GitHub:

https://github.com/myduoy/Crimson-Desert-AutoLoot-CN/issues

