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
    assert 2 not in values, "type 2 is too broad and must be prompt-gated"
    assert 15 in values, "original AutoLoot corpse/gather type 15 must be handled"
    assert 168 in values, "rewrite-observed corpse type 168 must stay handled"
    assert 38 not in values, "DeadAnimal_Catch must not be treated as corpse loot"
    assert 39 not in values, "Animal_Catch must not be treated as corpse loot"


def test_type2_corpse_prompt_is_gated_by_action_text() -> None:
    matcher = body_of("IsPromptGatedCorpseType")
    assert "type == 2" in matcher

    fallback = body_of("IsCorpseInteraction")
    gated_index = fallback.find("IsPromptGatedCorpseType")
    recent_index = fallback.find("HasRecentCorpsePromptAction(now)", gated_index)
    unsafe_index = fallback.find("IsUnsafePromptActionFallbackType")
    assert gated_index != -1, "type 2 must be handled explicitly"
    assert recent_index != -1, "type 2 must require a recent corpse prompt action"
    assert unsafe_index == -1 or gated_index < unsafe_index


def test_type2_ground_prompt_is_resolved_through_item_filter() -> None:
    matcher = body_of("IsPromptGatedGroundType")
    assert "type == 2" in matcher

    helper = body_of("IsGroundInteraction")
    assert "IsGroundLootType(type)" in helper
    assert "IsPromptGatedGroundType(type)" in helper
    assert "HasRecentGroundPromptAction(now)" not in helper

    record = body_of("RecordInteraction")
    corpse_index = record.find("const bool corpse_interaction = IsCorpseInteraction(type, now)")
    ground_index = record.find("const bool ground_interaction =")
    assert corpse_index != -1, "corpse classifier must run for type 2 prompts"
    assert ground_index != -1, "ground classifier must still run"
    assert corpse_index < ground_index, "corpse prompts must not be swallowed by type 2 ground fallback"
    assert "!corpse_interaction && IsGroundInteraction(type, now)" in record
    assert "if (ground_interaction &&" in record
    assert "QueueGroundResolveRequest(type, target, candidate, context, now)" in record
    assert "IsPromptGatedGroundType(type) && !HasRecentGroundPromptAction(now)" in record

    processor = body_of("ProcessGroundResolveRequest")
    assert "IsPromptGatedGroundType(request.type) &&" in processor
    assert "!HasRecentGroundPromptAction(now)" in processor


def test_ground_prompt_action_text_includes_take() -> None:
    matcher = body_of("IsGroundPromptActionText")
    assert "\\x62FF\\x53D6" in matcher  # 拿取
    assert 'lower == L"take"' in matcher
    assert 'lower == L"pick up"' in matcher
    assert 'lower == L"pickup"' in matcher


def test_prompt_action_gates_reset_opposites() -> None:
    corpse = body_of("StoreCorpsePromptAction")
    ground = body_of("StoreGroundPromptAction")
    assert "g_recent_ground_prompt_action, 0" in corpse
    assert "g_recent_corpse_prompt_action, 0" in ground


def test_prompt_gated_ground_type_blocks_misc_scene_props() -> None:
    helper = body_of("IsPromptGatedGroundCategoryAllowed")
    assert "kCatMisc" in helper
    assert "return false" in helper

    processor = body_of("ProcessGroundResolveRequest")
    gated_index = processor.find("IsPromptGatedGroundType(request.type)")
    category_index = processor.find("IsPromptGatedGroundCategoryAllowed(category)")
    assert gated_index != -1, "prompt-gated ground types need category guard"
    assert category_index != -1, "prompt-gated ground types must reject scene props"
    assert gated_index < category_index


def test_prompt_gated_ground_type_uses_short_prompt_cache() -> None:
    prompt = body_of("ResolveGroundItemFromPrompt")
    assert "max_age_ms" in prompt
    assert "now - snapshot.tick > max_age_ms" in prompt

    cached = body_of("ResolveGroundItemCached")
    assert "prompt_max_age_ms" in cached
    assert "IsPromptGatedGroundType(type) ? kPromptActionMatchTtlMs" in cached


def test_corpse_prompt_action_text_includes_skinning() -> None:
    matcher = body_of("IsCorpsePromptActionText")
    assert "\\x5265\\x76AE" in matcher  # 剥皮
    assert 'lower == L"skin"' in matcher
    assert 'lower == L"skinning"' in matcher


def test_skinning_types_use_hold_interact_path() -> None:
    match = re.search(r"kHoldInteractTypes\[\]\s*=\s*\{([^}]+)\}", SOURCE)
    assert match, "hold interaction IDs should live in kHoldInteractTypes[]"
    values = {int(number) for number in re.findall(r"\b\d+\b", match.group(1))}
    assert 160 in values, "original skinning A type must trigger hold interact"
    assert 161 in values, "original skinning B type must trigger hold interact"
    assert 171 in values, "May 2026 skinning type must trigger hold interact"
    assert 172 in values, "May 11 skinning A type must trigger hold interact"
    assert 173 in values, "May 11 skinning B type must trigger hold interact"


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
        37, 38, 39, 40, 41, 42, 50, 160, 161, 171, 172, 173, 266, 93
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
    assert "kCorpseInteractHoldMs=900" in source_no_spaces

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
    for category in (
        "kCatHelmet",
        "kCatChestArmor",
        "kCatGloves",
        "kCatBoots",
        "kCatCloak",
        "kCatShield",
        "kCatTowerShield",
        "kCatBow",
        "kCatOneHandWeapon",
        "kCatTwoHandWeapon",
    ):
        assert category in generic
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
    test_type2_corpse_prompt_is_gated_by_action_text()
    test_type2_ground_prompt_is_resolved_through_item_filter()
    test_ground_prompt_action_text_includes_take()
    test_prompt_action_gates_reset_opposites()
    test_prompt_gated_ground_type_blocks_misc_scene_props()
    test_prompt_gated_ground_type_uses_short_prompt_cache()
    test_corpse_prompt_action_text_includes_skinning()
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
