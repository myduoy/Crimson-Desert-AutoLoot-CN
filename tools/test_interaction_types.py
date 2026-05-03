from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[1]
SOURCE = (ROOT / "src" / "main.cpp").read_text(encoding="utf-8")


def body_of(function_name: str) -> str:
    marker = f"{function_name}("
    start = SOURCE.find(marker)
    assert start != -1, f"{function_name} is missing"
    brace = SOURCE.find("{", start)
    assert brace != -1, f"{function_name} has no body"
    depth = 0
    for index in range(brace, len(SOURCE)):
        char = SOURCE[index]
        if char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                return SOURCE[brace:index + 1]
    raise AssertionError(f"{function_name} body is unterminated")


def test_corpse_types_include_original_and_rewrite_ids() -> None:
    match = re.search(r"kCorpseLootTypes\[\]\s*=\s*\{([^}]+)\}", SOURCE)
    assert match, "corpse interaction IDs should live in kCorpseLootTypes[]"
    values = {int(number) for number in re.findall(r"\b\d+\b", match.group(1))}
    assert 15 in values, "original AutoLoot corpse/gather type 15 must be handled"
    assert 168 in values, "rewrite-observed corpse type 168 must stay handled"
    assert 38 not in values, "DeadAnimal_Catch must not be treated as corpse loot"
    assert 39 not in values, "Animal_Catch must not be treated as corpse loot"


def test_skinning_types_use_hold_interact_path() -> None:
    match = re.search(r"kHoldInteractTypes\[\]\s*=\s*\{([^}]+)\}", SOURCE)
    assert match, "hold interaction IDs should live in kHoldInteractTypes[]"
    values = {int(number) for number in re.findall(r"\b\d+\b", match.group(1))}
    assert 160 in values, "original skinning A type must trigger hold interact"
    assert 161 in values, "original skinning B type must trigger hold interact"
    assert 171 in values, "current-client skinning type must trigger hold interact"


def test_record_interaction_uses_corpse_classifier() -> None:
    body = body_of("RecordInteraction")
    assert "IsCorpseInteraction" in body
    assert "type == kCorpseLootType" not in body


def test_observed_current_ground_type_is_handled() -> None:
    matcher = body_of("IsGroundLootType")
    assert "kGroundLootCurrentType" in matcher

    safety = body_of("IsUnsafePromptActionFallbackType")
    assert "case 5:" in safety


def test_prompt_action_fallback_is_guarded() -> None:
    fallback = body_of("IsCorpseInteraction")
    assert "HasRecentCorpsePromptAction" in fallback
    assert "IsUnsafePromptActionFallbackType" in fallback
    safety = body_of("IsUnsafePromptActionFallbackType")
    for interaction_type in (
        1, 4, 19, 24, 25, 26, 27, 29, 30, 31, 32, 33, 34, 35, 36,
        37, 38, 39, 40, 41, 42, 50, 160, 161, 266, 93
    ):
        assert f"case {interaction_type}:" in safety


def test_type1_unknown_ground_can_fall_back_to_corpse() -> None:
    helper = body_of("ShouldFallbackGroundTypeToCorpse")
    assert "type != kGroundLootType" in helper
    assert "HasRecentCorpsePromptAction(now)" in helper
    assert "!item.text_match" in helper
    assert "item.unique_keys > 1" in helper
    assert "IsReliableFilteredGroundItem" in helper
    assert "g_corpse_enabled" in helper

    processor = body_of("ProcessGroundResolveRequest")
    filtered_index = processor.find("InterlockedIncrement64(&g_filtered")
    fallback_index = processor.find("ShouldFallbackGroundTypeToCorpse")
    assert fallback_index != -1, "ground filtering must test corpse fallback"
    assert filtered_index != -1, "ground filtering must still record filtered items"
    assert fallback_index < filtered_index, "corpse fallback must run before filtering"
    assert "g_pending_corpse" in processor


def test_blocked_short_pointer_text_cannot_override_allowed_equipment_category() -> None:
    helper = body_of("ShouldUseNumericCategoryOverTextRefine")
    assert "text.source != 5" in helper
    assert "text.score > 1003" in helper
    assert "IsEquipmentLikeCategory" in helper
    assert "IsItemCategoryAllowed(numeric_category)" in helper
    assert "IsItemBlocked(text.key)" in helper

    refiner = body_of("bool TryGroundTextRefine")
    assert "ShouldUseNumericCategoryOverTextRefine" in refiner
    assert "source = 13" in refiner
    assert "key = 0" in refiner


def test_corpse_interaction_uses_hold_not_tap() -> None:
    source_no_spaces = re.sub(r"\s+", "", SOURCE)
    assert "kGroundInteractTapMs=55" in source_no_spaces
    assert re.search(r"kCorpseInteractHoldMs\s*=\s*(?:[89]\d{2}|1\d{3})", SOURCE)

    press = body_of("PressInteractKey")
    assert "hold_ms" in press
    assert "Sleep(hold_ms)" in press

    processor = body_of("ProcessPendingInput")
    assert "has_corpse ? kCorpseInteractHoldMs : kGroundInteractTapMs" in processor
    assert "PressInteractKey(hold_ms)" in processor


def test_generic_equipment_prompt_text_can_classify_category() -> None:
    matcher = body_of("MatchItemNameText")
    assert "TryMatchGenericItemCategoryText" in matcher

    generic = body_of("TryMatchGenericItemCategoryText")
    assert "kCatArmor" in generic
    assert "kCatWeapon" in generic
    assert "FillCategoryTextMatchResult" in generic
    assert "\\x677F\\x91D1\\x5934\\x76D4" in generic  # 板金头盔
    assert "Plate Helm" in generic


def test_category_only_text_match_is_reliable_for_filtering() -> None:
    reliable = body_of("IsReliableFilteredGroundItem")
    assert "item.text_match" in reliable
    assert "item.category != kCatUnknown" in reliable
    assert "item.key != 0 ||" in reliable


if __name__ == "__main__":
    test_corpse_types_include_original_and_rewrite_ids()
    test_skinning_types_use_hold_interact_path()
    test_record_interaction_uses_corpse_classifier()
    test_observed_current_ground_type_is_handled()
    test_prompt_action_fallback_is_guarded()
    test_type1_unknown_ground_can_fall_back_to_corpse()
    test_blocked_short_pointer_text_cannot_override_allowed_equipment_category()
    test_corpse_interaction_uses_hold_not_tap()
    test_generic_equipment_prompt_text_can_classify_category()
    test_category_only_text_match_is_reliable_for_filtering()
    print("interaction type tests passed")
