import ctypes
import csv
import json
import re
import shutil
import sqlite3
from collections import Counter
from datetime import datetime
from pathlib import Path


WEAPON_SLOTS = {
    0, 1, 2, 3, 14, 15, 16, 17, 18, 19, 20, 21, 23, 24, 28, 29, 30, 31,
    32, 33, 34, 35, 36, 37, 38, 39, 43, 47, 48, 49, 50, 51, 52,
}
ARMOR_SLOTS = {4, 5, 6, 7, 11, 69}
ACCESSORY_SLOTS = {8, 9, 10, 70, 71, 72}
TOOL_SLOTS = {55, 84, 85, 86, 87, 88, 89, 90, 91, 92, 94, 95, 96, 97, 99, 101}
HORSE_GEAR_SLOTS = {60, 61, 62, 63, 64}
PET_GEAR_SLOTS = {58, 59}
VEHICLE_GEAR_SLOTS = {73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83}
EQUIPMENT_SLOTS = {13, 66}


def support_dir() -> Path:
    return Path(__file__).resolve().parent


def save_editor_dir() -> Path:
    return support_dir().parent.parent / "Save Editor"


def system_language_code() -> str:
    lang_id = ctypes.windll.kernel32.GetUserDefaultUILanguage()
    return {
        0x0404: "zh-tw",
        0x0804: "zh",
        0x0411: "ja",
        0x0412: "ko",
        0x0407: "de",
        0x0C0A: "es",
        0x040A: "es",
        0x080A: "es-mx",
        0x040C: "fr",
        0x0410: "it",
        0x0415: "pl",
        0x0416: "pt-br",
        0x0816: "pt-br",
        0x0419: "ru",
        0x041F: "tr",
    }.get(lang_id, "en")


def load_localized_names(locale_dir: Path, language: str) -> tuple[dict[str, str], str, Path | None]:
    candidates = []
    if language != "en":
        candidates.append(locale_dir / f"names_{language}.json")
    candidates.append(locale_dir / "names_zh.json")

    for path in candidates:
        if not path.exists():
            continue
        data = json.loads(path.read_text(encoding="utf-8"))
        return data.get("items", {}), data.get("_language_name", language), path
    return {}, "English fallback", None


def classify_item(row: sqlite3.Row) -> str:
    key = int(row["item_key"])
    name = row["name"] or ""
    internal = row["internal_name"] or ""
    db_category = row["category"] or ""
    icon = row["icon_fallback"] or ""
    data = json.loads(row["limit_data"] or "{}")
    slot_type = int(data.get("slotType", 65535))
    inventory_category = int(data.get("inventoryCategory", 2))
    text = f"{name} {internal} {icon}".lower()
    lower_name = name.lower()
    lower_internal = internal.lower()
    internal_tokens = set(re.split(r"[^a-z0-9]+", lower_internal))

    if db_category == "Quest" or lower_internal.startswith("quest_"):
        return "Quest"

    if (
        db_category == "Currency"
        or lower_internal.startswith("money_")
        or lower_internal in {
            "goldbar",
            "copper_pack",
            "silver_pack",
            "heavy_copper_pack",
            "small_copper_pack",
            "heavy_silver_pack",
        }
        or ("coin" in text and inventory_category == 2)
    ):
        return "Currency"

    if (
        lower_internal.startswith("recipe_")
        or lower_internal.startswith("craftingrecipe_")
        or "blueprint" in lower_internal
        or "blueprint" in lower_name
        or lower_name.startswith("recipe:")
        or lower_name.startswith("alchemy formula")
    ):
        return "Recipe"

    if (
        lower_internal.startswith(("book_", "paper_", "noticepaper_", "lostletter_", "treasuremap_"))
        or any(token in lower_name for token in (" manual", " report", " record", " flyer", " letter", " poster", " map piece"))
        or lower_name.endswith(" map")
    ):
        return "Document"

    if lower_internal.startswith("trade_") or lower_name.startswith("packaged "):
        return "Trade"

    if slot_type in HORSE_GEAR_SLOTS or "horsearmor" in lower_internal or "horseshoe" in lower_internal:
        return "HorseGear"

    if slot_type in PET_GEAR_SLOTS or "petarmor" in lower_internal:
        return "PetGear"

    if slot_type in VEHICLE_GEAR_SLOTS or "warrobot" in lower_internal:
        return "VehicleGear"

    armor_keywords = (
        "platearmor",
        "leather_armor",
        "fabric_armor",
        "chainmail",
        "_helm",
        "_gloves",
        "_boots",
        "_cloak",
    )
    if any(keyword in lower_internal for keyword in armor_keywords):
        return "Armor"

    if slot_type in WEAPON_SLOTS:
        return "Weapon"

    if slot_type in ARMOR_SLOTS:
        return "Armor"

    if slot_type in ACCESSORY_SLOTS:
        return "Accessory"

    if slot_type in TOOL_SLOTS:
        return "Tool"

    if slot_type in EQUIPMENT_SLOTS:
        return "Equipment"

    if (
        "cannonball" in lower_internal
        or "cannonball" in lower_name
        or lower_internal in {
            "arrow",
            "poison_arrow",
            "sleep_arrow",
            "bomb_arrow",
            "fire_arrow",
            "ice_arrow",
            "lightning_arrow",
            "multiarrow",
            "quiver",
        }
        or (slot_type == 65535 and re.search(r"(^|_)arrow($|_)", lower_internal))
    ):
        return "Ammo"

    if (
        "horsefeed" in lower_internal
        or "potion" in lower_internal
        or "remedy" in lower_internal
        or "elixir" in lower_name
        or "feed" in lower_internal
    ):
        return "Consumable"

    if (
        lower_internal.startswith(("food_", "grilled_", "braised_", "pan_fried_", "roast_"))
        or "cookfailed" in lower_internal
        or db_category == "Consumable" and not lower_internal.startswith("recipe_")
    ):
        return "Food"

    food_tokens = {
        "food_",
        "grilled_",
        "braised_",
        "pan_fried_",
        "roast_",
        "seafood",
        "soup",
        "porridge",
        "sanjeok",
        "stew",
        "tea",
        "juice",
        "wine",
        "beer",
        "water",
        "milk",
        "jerky",
        "honey",
        "cheese",
        "fish",
        "crab",
        "shrimp",
    }
    if internal_tokens.intersection(food_tokens):
        return "Food"

    if (
        db_category == "Material"
        or 700000 <= key < 760000
        or "material" in lower_internal
        or lower_internal in {"fine_stone", "premium_stone"}
    ):
        return "Material"

    if db_category == "Equipment":
        return "Equipment"

    if db_category == "Ammo":
        return "Ammo"

    return "Misc"


def read_items(db_path: Path) -> list[sqlite3.Row]:
    con = sqlite3.connect(db_path)
    con.row_factory = sqlite3.Row
    return con.execute(
        """
        select
          i.item_key,
          i.name,
          i.internal_name,
          i.category,
          coalesce(il.data, '{}') as limit_data,
          coalesce(ii.icon_fallback, '') as icon_fallback
        from items i
        left join item_limits il on il.item_key = i.item_key
        left join item_icons ii on ii.item_key = i.item_key
        order by i.item_key
        """
    ).fetchall()


def main() -> None:
    out_path = support_dir() / "crimson_autoloot_items.tsv"
    backups = support_dir() / "backups"
    editor = save_editor_dir()
    db_path = editor / "crimson_data.db"
    locale_dir = editor / "locale"

    language = system_language_code()
    localized_names, language_name, language_path = load_localized_names(locale_dir, language)
    rows = read_items(db_path)

    if out_path.exists():
        backups.mkdir(exist_ok=True)
        stamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        shutil.copy2(out_path, backups / f"crimson_autoloot_items.tsv.{stamp}.bak")

    counts: Counter[str] = Counter()
    with out_path.open("w", encoding="utf-8", newline="") as f:
        source = str(language_path) if language_path else "database english names"
        f.write(f"# locale={language}\tlanguage={language_name}\tsource={source}\n")
        f.write("# item_key\tcategory\tcurrent_language_name\tenglish_name\tinternal_name\tslotType\tinventoryCategory\n")
        writer = csv.writer(f, delimiter="\t", lineterminator="\n")
        for row in rows:
            data = json.loads(row["limit_data"] or "{}")
            category = classify_item(row)
            counts[category] += 1
            key = int(row["item_key"])
            writer.writerow(
                [
                    key,
                    category,
                    localized_names.get(str(key), ""),
                    row["name"] or "",
                    row["internal_name"] or "",
                    int(data.get("slotType", 65535)),
                    int(data.get("inventoryCategory", 2)),
                ]
            )

    print(f"wrote {out_path}")
    print(f"language={language} name={language_name} items={len(rows)}")
    for category, count in sorted(counts.items()):
        print(f"{category}\t{count}")


if __name__ == "__main__":
    main()
