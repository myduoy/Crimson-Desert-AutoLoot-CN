# Changelog

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
