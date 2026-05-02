# Changelog

## v0.1.6

- Updated the prompt text and interaction hook addresses for the May 2, 2026 Crimson Desert game update.
- Added observed current-client ground loot interaction `type 5`, fixing prompts that were logged as seen but never triggered.
- Added a local regression test that scans the installed `CrimsonDesert.exe` and verifies hook constants, derived return targets, prompt literal target, and prompt text call targets.
- Fixed PE raw-file-offset to runtime-RVA conversion for current game builds where section raw offsets are not equal to RVAs.
- Updated release binaries.

## v0.1.5

- Fixed configuration UI integer and checkbox reads for UTF-8 BOM INI files.
- Prevented the configuration UI from falling back to default checkbox values and rewriting them when WinAPI cannot read the first INI section.
- Added regression coverage for the shared manual INI fallback path.

## v0.1.4

- Fixed `InteractKey` changes not working for full key names such as `Space`, `Tab`, `Insert`, `Delete`, arrow keys, and function keys.
- Stopped the configuration UI from truncating `InteractKey` to the first character.
- Added automatic Chinese/English configuration UI language detection: Chinese Windows defaults to Chinese, other systems default to English.
- Added a language selector in the configuration UI with `Auto`, `Chinese`, and `English`.
- Added English category labels, UI text, status messages, and English item-name display.
- Added `Language=Auto` to the default configuration.
- Added regression tests for interact-key parsing and UI language switching.

## v0.1.3

- Fixed small animals such as geese and hedgehogs being caught by mistake when corpse looting was enabled.
- Removed animal catch interaction IDs `38` and `39` from the corpse-loot classifier.
- Tightened the ambiguous `type 1` corpse fallback so it only runs after a recent corpse/search prompt action is seen.
- Expanded unsafe prompt-action fallback guards for animal, critter, insect, gather, greeting, skinning, chest, and carry interaction types.
- Updated interaction regression tests and release binaries.

## v0.1.2

- Fixed human NPC corpse looting on current game builds where corpse prompts can be reported as ground interaction `type 1`.
- Changed corpse interaction input from a short tap to a held interact key so "search/loot corpse" prompts complete reliably.
- Added current-client corpse interaction candidates `38`, `39`, and `168` while keeping the original corpse/gather type.
- Added a safe fallback for ambiguous `type 1` corpse prompts without bypassing item filters for confirmed ground items.
- Improved ground item filtering so unknown numeric object matches are not treated as real item IDs.
- Added generic equipment prompt classification for names such as faction-prefix plate helmets, shields, spears, swords, bows, armor, gloves, boots, and cloaks.
- Added regression tests for interaction type handling, corpse key hold behavior, and generic equipment category matching.
- Updated release binaries.

## v0.1.1

- Improved item filtering reliability by requiring confirmed item-name resolution when filters are enabled.
- Added candidate-string resolution before broad memory text scanning.
- Moved expensive ground-item text resolution out of the game hook callback and into the plugin worker thread.
- Reduced repeated text scans for cached ground-item results.
- Fixed `DebugLog=0` so runtime logging can be disabled completely.
- Kept combat-pause / weapon-state probing disabled for stability.

## v0.1.0

- Added Chinese AutoLoot ASI plugin.
- Added Chinese configuration UI.
- Added ground loot and corpse interaction automation.
- Added category-based item filter and per-item block list.
- Added hotkeys for plugin toggle and config panel.
- Removed combat-pause and weapon-state probing logic from the public build.
