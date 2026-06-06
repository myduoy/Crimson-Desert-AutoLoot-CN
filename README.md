# Crimson Desert AutoLoot CN

Crimson Desert AutoLoot CN is a Windows x64 ASI plugin for Crimson Desert. It adds automatic ground loot, automatic corpse looting, item filters, hotkeys, and a Chinese configuration UI.

This is an unofficial community plugin. It does not include game files and is not affiliated with Pearl Abyss or the official Crimson Desert team.

## Features

- Automatic ground loot by sending the configured interact key when a loot prompt is detected.
- Automatic corpse looting for search/loot corpse prompts.
- Category-based item filtering and per-item block list.
- Chinese/English configuration UI for feature toggles, hotkeys, item search, logs, and item table browsing.
- Automatic UI language detection, plus manual `Auto` / `Chinese` / `English` selection.
- Localized item names in the item table, with fallback to English or internal names when a translation is missing.
- Hot-reloaded configuration when the INI file changes.
- Lower-stutter filtering path: expensive ground-item text resolution runs on the plugin worker thread instead of the game hook callback.

## User Guide

Installation, runtime requirements, update, troubleshooting, and uninstall documentation:

```text
docs\INSTALL_UNINSTALL.en-US.md
docs\INSTALL_UNINSTALL.zh-CN.md
```

ASI loader download:

```text
https://github.com/ThirteenAG/Ultimate-ASI-Loader/releases/tag/v9.7.1
```

## v0.1.17 Fixes

- Updated the supported build timestamp for the June 6, 2026 Crimson Desert game update.
- Refreshed the prompt resolver target and expected resolver bytes for the current client.
- Kept prompt text, prompt branch, skip, literal, and call targets unchanged because they still match the current executable.
- Updated release binaries and package.

## v0.1.16 Fixes

- Updated the supported build timestamp for the June 5, 2026 Crimson Desert game update.
- Updated prompt update, prompt text A/B, prompt branch, skip target, literal target, and call target constants for the current installed game executable.
- Refreshed prompt text and resolver expected bytes for the current client while keeping the existing `bpl` branch-hook behavior.
- Updated release binaries and package.

## v0.1.15 Fixes

- Updated the supported build timestamp for the May 30, 2026 Crimson Desert game update.
- Updated prompt update, prompt text A/B, prompt branch, skip target, literal target, and call target constants for the current installed game executable.
- Updated the prompt branch hook stub for the current `bpl` interaction-type register allocation.
- Expanded hook regression coverage so future game updates can detect the relaxed prompt text signature, backward branch search, and resolver-call layout.
- Updated release binaries and package.

## v0.1.14 Fixes

- Fixed current-client `type 2` ground equipment prompts so filtered ground loot can pick up visible gloves, armor, helmets, shields, and weapons without enabling broad scene interactions.
- Kept `type 2` scene props guarded by text and category checks so chairs, cooking pots, wall notes, and other non-loot interactions are not auto-triggered.
- Reclassified high-ID equipment rows whose database `slotType` is missing by using internal item-name fallbacks for helmets, chest armor, gloves, boots, cloaks, shields, bows, and weapons.
- Added regression coverage for prompt-gated ground filtering and equipment category fallback.
- Updated release binaries and package.

## v0.1.13 Fixes

- Updated the supported build timestamp for the May 22, 2026 Crimson Desert game update.
- Updated prompt update, prompt text A/B, prompt branch, skip target, literal target, and call target constants for the current installed game executable.
- Updated prompt text and branch hook stubs for the current prompt owner/context register allocation.
- Added the current-client corpse prompt interaction `type 2` to the long-hold corpse loot path.
- Expanded hook regression coverage for prompt update and branch expected bytes.
- Updated release binaries.

## v0.1.12 Fixes

- Updated the supported build timestamp for the May 15, 2026 Crimson Desert game update.
- Updated prompt update, prompt text A/B, prompt branch, skip target, literal target, and call target constants for the current installed game executable.
- Updated release binaries.

## v0.1.11 Fixes

- Updated the supported build timestamp for the May 12, 2026 Crimson Desert game update.
- Updated the prompt text A literal target and expected hook bytes for the current installed game executable.
- Updated release binaries.

## v0.1.10 Fixes

- Fixed skinning after the May 11, 2026 game update by adding observed interaction types `172` and `173` to the long-hold interaction path.
- Added regression coverage for the new skinning interaction IDs.
- Updated release binaries.

## v0.1.9 Fixes

- Updated the supported build timestamp and prompt hook addresses for the May 11, 2026 Crimson Desert game update.
- Updated prompt update, prompt text A/B, prompt branch, skip target, literal target, and call target constants for the current installed game executable.
- Added regression coverage for the prompt update hook entry.
- Updated release binaries.

## v0.1.8 Fixes

- Added English and Chinese installation, runtime requirements, update, troubleshooting, and uninstall documentation.
- Documented Ultimate ASI Loader v9.7.1 as the recommended ASI loader source.
- Fixed in-game F9/F10 status toasts so English UI users see English messages.
- Updated the release package to include the `docs` directory.

## v0.1.7 Fixes

- Updated the supported build timestamp and prompt hook addresses for the May 3, 2026 Crimson Desert game update.
- Verified the hook constants against the installed game executable with the local regression scanner.
- Fixed the prompt text A installation guard so corpse looting can see search/loot corpse prompt text again.
- Added current-client skinning interaction `type 171`.
- Reduced false filtering where stale nearby material text could block visible weapon/armor/tool pickups.
- Updated release binaries.

## v0.1.6 Fixes

- Updated the prompt text and interaction hook addresses for the May 2, 2026 Crimson Desert game update.
- Added observed current-client ground loot interaction `type 5`, fixing loot prompts that were seen but never triggered.
- Added a local regression test that scans the installed game executable and checks the plugin hook constants against the current build.
- Fixed the hook scanner/test path to convert PE raw file offsets to runtime RVAs before validating addresses.
- Updated release binaries.

## v0.1.5 Fixes

- Fixed configuration UI checkbox and integer settings when the INI file is saved as UTF-8 with BOM.
- Prevented the config window from showing default checkbox states and rewriting them over existing values in that case.
- Updated release binaries.

## v0.1.4 Fixes

- Fixed `InteractKey` changes not working for full key names such as `Space`, `Tab`, `Insert`, `Delete`, arrow keys, and function keys.
- Stopped the configuration UI from truncating the interact key to one character.
- Added Chinese/English automatic language detection and a language selector.
- Added English UI labels, category names, status messages, and English item-name display.
- Added `Language=Auto` to the default configuration.
- Updated release binaries.

## v0.1.3 Fixes

- Fixed small animals such as geese and hedgehogs being caught by mistake.
- Removed animal catch interaction IDs `38` and `39` from corpse looting.
- Tightened ambiguous `type 1` corpse fallback so it only runs after a recent corpse/search prompt action is seen.
- Kept human NPC corpse looting and generic equipment prompt filtering from `v0.1.2`.
- Added regression coverage for animal/catch interaction safety guards.
- Updated release binaries.

## v0.1.2 Fixes

- Fixed human NPC corpse looting on current game builds.
- Fixed corpse prompts that are reported by the game as ground interaction `type 1`.
- Changed corpse interaction input from a short tap to a held interact key so search/loot corpse prompts complete reliably.
- Added current-client corpse interaction candidates `38`, `39`, and `168`.
- Added a safe fallback for ambiguous `type 1` corpse prompts without bypassing filters for confirmed ground items.
- Improved filtering so unknown numeric object matches are not treated as real item IDs.
- Added generic equipment prompt classification for faction-prefixed item names such as plate helmets, helmets, armor, gloves, boots, cloaks, shields, spears, swords, and bows.
- Added regression tests for interaction type handling, corpse key hold behavior, and generic equipment category matching.
- Updated release binaries.

## Current Limitations

The public build does not include "pause auto-loot in combat". Experimental combat-state, weapon-state, and UI scanning code was removed for stability.

Debug logging is disabled by default. Set `DebugLog=1` in `crimson_autoloot_cn.ini` only when troubleshooting.

## Files

- `release/crimson_autoloot_cn.asi`: in-game ASI plugin.
- `release/crimson_autoloot_config.exe`: configuration UI.
- `src/main.cpp`: ASI plugin source.
- `src/config_ui.cpp`: configuration UI source.
- `crimson_autoloot_defaults.ini`: default configuration.
- `crimson_autoloot_items.tsv`: item category and name table.
- `tools/generate_items.py`: item table generation script.
- `tools/test_current_game_hooks.py`: local hook-address regression test against the installed game executable.
- `tools/test_interaction_types.py`: regression tests for interaction and filtering behavior.
- `build.ps1`: local build script.
- `package.ps1`: release zip packaging script.

## Install

For the full guide, see `docs\INSTALL_UNINSTALL.en-US.md` or `docs\INSTALL_UNINSTALL.zh-CN.md`.

1. Close the game.
2. Copy `release/crimson_autoloot_cn.asi` to:

   ```text
   <Crimson Desert>\bin64\crimson_autoloot_cn.asi
   ```

3. Create the support directory:

   ```text
   <Crimson Desert>\bin64\crimson_autoloot_cn\
   ```

4. Copy these files into the support directory:

   ```text
   crimson_autoloot_config.exe
   crimson_autoloot_defaults.ini
   crimson_autoloot_items.tsv
   ```

5. Start the game. Default keys:

   ```text
   F9  Toggle auto-loot
   F10 Open/close config panel
   E   Game interact key
   ```

## Configuration

Run `crimson_autoloot_config.exe`, or press `F10` in game to open the config panel.

Main options:

- `Enable`: master switch.
- `Ground`: automatic ground item looting.
- `Corpse`: automatic corpse looting/searching.
- `Filter`: item filter switch.
- `Foreground only`: only send keys when the game window is focused.
- `Interact key`: defaults to `E`; should match the game's basic interaction key.
- `Toggle key`: defaults to `F9`.
- `Panel key`: defaults to `F10`.

Filter behavior:

- A checked category means items in that category are allowed.
- Unchecking a category blocks all items in that category.
- Unchecking a single item adds that item to the block list.
- The search box filters items within the current category.
- If a game prompt only reveals a generic equipment category and not a specific item key, the plugin applies the category switch.

## Build From Source

Requires Visual Studio Build Tools with the C++ toolchain.

```powershell
powershell -ExecutionPolicy Bypass -File .\build.ps1
```

Build outputs:

```text
build\crimson_autoloot_cn.asi
build\crimson_autoloot_config.exe
```

## Tests

```powershell
python .\tools\test_interaction_types.py
```

## Package

```powershell
powershell -ExecutionPolicy Bypass -File .\package.ps1
```

Output:

```text
dist\
```

## Logs

Runtime log path:

```text
<Crimson Desert>\bin64\crimson_autoloot_cn\crimson_autoloot_cn.log
```

If auto-loot does not work, check:

- The game interact key still matches the plugin interact key.
- The plugin is enabled.
- The item category or individual item is not filtered.
- The game window is focused when `Foreground only` is enabled.

## License

MIT License.
