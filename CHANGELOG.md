# Changelog

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
