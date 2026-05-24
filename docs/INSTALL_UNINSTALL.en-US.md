# Install, Runtime Requirements, and Uninstall Guide

This guide explains the runtime requirements, installation steps, update flow, troubleshooting, and uninstall steps for Crimson Desert AutoLoot CN.

## Runtime Requirements

- Game: Steam version of Crimson Desert, Windows x64 client.
- Plugin folder: the game's `bin64` directory.
- System: Windows 10 / Windows 11 64-bit.
- ASI loader: an ASI Loader capable of loading `.asi` plugins is required.
- Default interact key: the game's "Basic Interaction" key defaults to `E`. The plugin `Interact key` must match the in-game key.

Recommended ASI loader:

```text
https://github.com/ThirteenAG/Ultimate-ASI-Loader/releases/tag/v9.7.1
```

Ultimate ASI Loader supports proxy DLL names such as `version.dll`. For Crimson Desert, this guide uses the `bin64\version.dll` install layout.

## Download Files

Download the release package from GitHub Releases or Nexus Mods, for example:

```text
Crimson-Desert-AutoLoot-CN-v0.1.14.zip
```

After extraction, the package should look like this:

```text
Crimson-Desert-AutoLoot-CN\
  crimson_autoloot_cn.asi
  crimson_autoloot_cn\
    crimson_autoloot_config.exe
    crimson_autoloot_defaults.ini
    crimson_autoloot_items.tsv
  docs\
    INSTALL_UNINSTALL.en-US.md
    INSTALL_UNINSTALL.zh-CN.md
  README.md
  LICENSE
```

## Install ASI Loader

Skip this section if you already have a working ASI Loader installed.

1. Close the game.
2. Open the Ultimate ASI Loader v9.7.1 release page:

   ```text
   https://github.com/ThirteenAG/Ultimate-ASI-Loader/releases/tag/v9.7.1
   ```

3. Download the 64-bit DLL. Crimson Desert is a 64-bit game, so do not use the 32-bit DLL.
4. Put the DLL into:

   ```text
   <Crimson Desert>\bin64\
   ```

5. If the downloaded DLL is not named `version.dll`, rename it to `version.dll` unless you intentionally use another supported proxy name.
6. If `version.dll` already exists, back it up first, for example:

   ```text
   version.dll.bak
   ```

Do not install multiple conflicting ASI loaders at the same time. Conflicting proxy DLLs may prevent the game from starting or may load plugins more than once.

## Install AutoLoot

1. Close the game.
2. Extract `Crimson-Desert-AutoLoot-CN-v0.1.14.zip`.
3. Copy `crimson_autoloot_cn.asi` to:

   ```text
   <Crimson Desert>\bin64\crimson_autoloot_cn.asi
   ```

4. Copy the whole `crimson_autoloot_cn` folder to:

   ```text
   <Crimson Desert>\bin64\crimson_autoloot_cn\
   ```

5. The final layout should be:

   ```text
   <Crimson Desert>\bin64\
     version.dll
     crimson_autoloot_cn.asi
     crimson_autoloot_cn\
       crimson_autoloot_config.exe
       crimson_autoloot_defaults.ini
       crimson_autoloot_items.tsv
   ```

6. Start the game.

## First Run

On first run, the plugin creates the active configuration file:

```text
<Crimson Desert>\bin64\crimson_autoloot_cn\crimson_autoloot_cn.ini
```

Default hotkeys:

```text
F9   Toggle auto-loot
F10  Open or close the configuration panel
E    Interact key; this must match the game's Basic Interaction key
```

If your in-game interaction key is not `E`, open the configuration panel, change `Interact key`, and save. The plugin hot-reloads the INI, so a game restart is usually not required.

## Configuration

Open the configuration in either of these ways:

- Press `F10` in game.
- Close the game and run:

  ```text
  <Crimson Desert>\bin64\crimson_autoloot_cn\crimson_autoloot_config.exe
  ```

Main options:

- `Enable`: master switch.
- `Ground`: automatic ground item looting.
- `Corpse`: automatic corpse looting/searching.
- `Filter`: category and per-item filtering.
- `Only when game is foreground`: only sends the interact key when the game window is focused.
- `Interact`: the game's interaction key.
- `Toggle`: auto-loot toggle hotkey.
- `Panel`: configuration panel hotkey.
- `Language`: `Auto`, `Chinese`, or `English`.

Filter behavior:

- A checked category allows that category.
- An unchecked category blocks every item in that category.
- An unchecked item adds that item to the per-item block list.
- The search box only filters the currently displayed item list; it does not change checked states.

## Update

1. Close the game.
2. Back up your active configuration:

   ```text
   <Crimson Desert>\bin64\crimson_autoloot_cn\crimson_autoloot_cn.ini
   ```

3. Replace these files with the new release files:

   ```text
   <Crimson Desert>\bin64\crimson_autoloot_cn.asi
   <Crimson Desert>\bin64\crimson_autoloot_cn\crimson_autoloot_config.exe
   <Crimson Desert>\bin64\crimson_autoloot_cn\crimson_autoloot_defaults.ini
   <Crimson Desert>\bin64\crimson_autoloot_cn\crimson_autoloot_items.tsv
   ```

4. Usually keep the old `crimson_autoloot_cn.ini` to preserve your hotkeys, categories, and item filters.
5. If configuration behaves incorrectly after an update, close the game, move the old `crimson_autoloot_cn.ini` away, and let the plugin generate a fresh default file.

## Logs and Troubleshooting

Runtime log path:

```text
<Crimson Desert>\bin64\crimson_autoloot_cn\crimson_autoloot_cn.log
```

Detailed logging is disabled by default. To troubleshoot, enable `Debug` in the configuration UI or set:

```ini
[General]
DebugLog=1
```

Common checks:

- Game does not start: move `crimson_autoloot_cn.asi` out of `bin64` and test the game again. Then check whether the ASI Loader is the wrong bitness or whether multiple loaders conflict.
- F9/F10 do not work: check whether `Toggle key` or `Panel key` conflicts with in-game keybinds.
- Ground items are not picked up: check `Enable`, `Ground`, `Filter`, category states, and per-item block states.
- Corpses are not looted: check that `Corpse` is enabled and that a search/loot corpse prompt is visible.
- Chinese text appears on an English system: set `Language` to `English` in the configuration UI and save.
- The plugin stops working after a game update: the game update may have changed hook addresses. Wait for a plugin update.

## Uninstall

1. Close the game.
2. Delete the plugin file:

   ```text
   <Crimson Desert>\bin64\crimson_autoloot_cn.asi
   ```

3. Delete the plugin support folder:

   ```text
   <Crimson Desert>\bin64\crimson_autoloot_cn\
   ```

4. If you installed ASI Loader only for this plugin, you can also delete:

   ```text
   <Crimson Desert>\bin64\version.dll
   ```

5. If another mod also depends on the same ASI Loader, do not delete `version.dll`.
6. If you backed up an original DLL such as `version.dll.bak`, restore it according to your backup state.

## Full Cleanup

To remove plugin configuration and logs, delete:

```text
<Crimson Desert>\bin64\crimson_autoloot_cn\crimson_autoloot_cn.ini
<Crimson Desert>\bin64\crimson_autoloot_cn\crimson_autoloot_cn.log
```

Deleting the support folder removes these files as well.

## Safety Notes

- This is an unofficial community plugin. It does not include game files and is not affiliated with Pearl Abyss.
- Back up any DLLs you add or replace in `bin64`.
- Do not overwrite `.asi`, `.dll`, or configuration program files while the game is running.
- If the game crashes after an update, move the `.asi` file out of `bin64` first and confirm whether the base game still starts.
