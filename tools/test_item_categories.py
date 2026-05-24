from pathlib import Path
import csv
import re


ROOT = Path(__file__).resolve().parents[1]
GENERATOR = (ROOT / "tools" / "generate_items.py").read_text(encoding="utf-8")
MAIN = (ROOT / "src" / "main.cpp").read_text(encoding="utf-8")
UI = (ROOT / "src" / "config_ui.cpp").read_text(encoding="utf-8")
DEFAULTS = (ROOT / "crimson_autoloot_defaults.ini").read_text(
    encoding="utf-8-sig"
)

EXPECTED_FINE_CATEGORIES = (
    "OneHandWeapon",
    "TwoHandWeapon",
    "Bow",
    "Shield",
    "TowerShield",
    "Helmet",
    "ChestArmor",
    "Gloves",
    "Boots",
    "Cloak",
    "Ring",
    "Necklace",
    "Earring",
    "Bracelet",
    "HeadAccessory",
    "FaceAccessory",
    "Backpack",
    "AbyssGear",
)

EXPECTED_ITEM_CATEGORIES = {
    "290017": "Shield",       # 西德蒙圆盾 / Sydmon Round Shield
    "290502": "TowerShield",  # Gilliam Large Shield
    "300014": "Bow",          # 西德蒙弓箭 / Sydmon Bow
    "200024": "OneHandWeapon",
    "240034": "TwoHandWeapon",
    "9400": "Helmet",
    "8700": "ChestArmor",
    "9302": "Gloves",
    "9201": "Boots",
    "1000032": "Cloak",
    "8504": "Earring",
    "8801": "Necklace",
    "8501": "Ring",
    "1001129": "Bracelet",
    "1000479": "HeadAccessory",
    "1000806": "FaceAccessory",
    "950017": "Backpack",
    "75001": "AbyssGear",
    "1000449": "AbyssGear",
    "1002634": "Helmet",
    "1003776": "OneHandWeapon",
    "1004047": "ChestArmor",
    "1004048": "Gloves",
    "10021700": "TwoHandWeapon",
    "1483457479": "Helmet",
    "1657061401": "Helmet",
    "1818402956": "ChestArmor",
    "1830330393": "Boots",
    "1876071146": "ChestArmor",
    "1918672776": "Helmet",
    "2019832942": "Gloves",
    "2048616564": "Helmet",
    "2053286856": "Helmet",
    "2058140396": "ChestArmor",
    "2062522806": "ChestArmor",
    "2094675372": "Gloves",
}


def load_categories() -> dict[str, str]:
    categories: dict[str, str] = {}
    with (ROOT / "crimson_autoloot_items.tsv").open(
        "r", encoding="utf-8-sig", newline=""
    ) as handle:
        for row in csv.reader(handle, delimiter="\t"):
            if not row or row[0].startswith("#"):
                continue
            categories[row[0]] = row[1]
    return categories


def test_generator_uses_project_root_and_finds_save_editor_candidates() -> None:
    assert "ROOT = Path(__file__).resolve().parents[1]" in GENERATOR
    assert "def save_editor_candidates" in GENERATOR
    assert r"D:\soft\Steam\steamapps\common\Crimson Desert\Save Editor" in GENERATOR


def test_generator_defines_fine_slot_mappings() -> None:
    for token in (
        "SHIELD_SLOTS",
        "TOWER_SHIELD_SLOTS",
        "BOW_SLOTS",
        "ONE_HAND_WEAPON_SLOTS",
        "TWO_HAND_WEAPON_SLOTS",
        "ARMOR_SLOT_CATEGORIES",
        "ACCESSORY_SLOT_CATEGORIES",
    ):
        assert token in GENERATOR
    for category in EXPECTED_FINE_CATEGORIES:
        assert f'"{category}"' in GENERATOR


def test_item_table_uses_fine_equipment_categories() -> None:
    categories = load_categories()
    for key, category in EXPECTED_ITEM_CATEGORIES.items():
        assert categories.get(key) == category, (
            f"item {key} should be {category}, got {categories.get(key)}"
        )


def test_internal_equipment_variants_do_not_fall_to_misc() -> None:
    bad: list[str] = []
    with (ROOT / "crimson_autoloot_items.tsv").open(
        "r", encoding="utf-8-sig", newline=""
    ) as handle:
        for row in csv.reader(handle, delimiter="\t"):
            if not row or row[0].startswith("#"):
                continue
            category, internal = row[1], row[4].lower()
            if category != "Misc":
                continue
            if re.search(
                r"(platearmor|chainmail|_helm(?:_|$)|_gloves?(?:_|$)|"
                r"_boots?(?:_|$)|twohandsword|onehandrapier)",
                internal,
            ):
                bad.append(f"{row[0]}:{row[2] or row[3]}:{row[4]}")

    assert not bad, "equipment-looking internal names still classified as Misc: " + ", ".join(bad[:20])


def test_runtime_ui_and_defaults_know_fine_categories() -> None:
    for category in EXPECTED_FINE_CATEGORIES:
        assert f'return "{category}"' in MAIN
        assert f'return L"{category}"' in MAIN
        assert f'"{category.lower()}") == 0' in MAIN
        assert f'L"{category}"' in UI
        assert re.search(fr"(?m)^{category}=", DEFAULTS)


if __name__ == "__main__":
    test_generator_uses_project_root_and_finds_save_editor_candidates()
    test_generator_defines_fine_slot_mappings()
    test_item_table_uses_fine_equipment_categories()
    test_internal_equipment_variants_do_not_fall_to_misc()
    test_runtime_ui_and_defaults_know_fine_categories()
    print("item category tests passed")
