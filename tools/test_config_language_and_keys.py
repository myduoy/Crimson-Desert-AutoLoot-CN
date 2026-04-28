from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[1]
MAIN = (ROOT / "src" / "main.cpp").read_text(encoding="utf-8")
UI = (ROOT / "src" / "config_ui.cpp").read_text(encoding="utf-8")
DEFAULTS = (ROOT / "crimson_autoloot_defaults.ini").read_text(
    encoding="utf-8-sig"
)


def body_of(source: str, function_name: str) -> str:
    marker = f"{function_name}("
    start = -1
    while True:
        start = source.find(marker, start + 1)
        assert start != -1, f"{function_name} is missing"
        brace = source.find("{", start)
        assert brace != -1, f"{function_name} has no body"
        semicolon = source.find(";", start, brace)
        if semicolon == -1:
            break
    depth = 0
    for index in range(brace, len(source)):
        char = source[index]
        if char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                return source[brace:index + 1]
    raise AssertionError(f"{function_name} body is unterminated")


def test_interact_key_uses_full_key_parser_not_first_character() -> None:
    body = body_of(MAIN, "ReadInteractKey")
    assert "ParseKeyName" in body
    assert "Trim(" in body
    assert "return kDefaultInteractKey" in body
    assert "for (wchar_t ch : value)" not in body

    parser = body_of(MAIN, "ParseKeyName")
    assert "fn <= 24" in parser
    for token in ("SPACE", "TAB", "INSERT", "DELETE"):
        assert token in parser


def test_config_ui_saves_normalized_interact_key_names() -> None:
    body = body_of(UI, "SaveUiToConfig")
    assert "NormalizeInteractKeyText" in body
    assert "WriteIniString(L\"General\", L\"InteractKey\", interact_key)" in body
    assert "key.resize(1)" not in body

    normalizer = body_of(UI, "NormalizeInteractKeyText")
    assert "IsValidKeyName" in normalizer
    key_validator = body_of(UI, "IsValidKeyName")
    assert "fn <= 24" in key_validator
    for token in ("SPACE", "TAB", "INSERT", "DELETE"):
        assert token in key_validator


def test_language_setting_exists_in_defaults_and_ui() -> None:
    assert re.search(r"(?m)^Language=Auto$", DEFAULTS)
    assert "IDC_LANGUAGE_COMBO" in UI
    assert "ReadIniString(L\"General\", L\"Language\"" in UI
    assert "WriteIniString(L\"General\", L\"Language\"" in UI
    assert "ReadIniStringManual" in UI
    assert "ReadIniStringManual(section, key, &manual)" in body_of(
        UI, "ReadIniString"
    )
    assert "GetUserDefaultUILanguage" in UI
    assert "ApplyLanguageToUi" in UI


def test_ui_item_and_category_text_can_switch_to_english() -> None:
    category_display = body_of(UI, "CategoryDisplay")
    assert "IsEnglishUi()" in category_display
    assert "category->english_label" in category_display

    item_name = body_of(UI, "CurrentLanguageItemName")
    assert "IsEnglishUi()" in item_name
    assert "item.english_name" in item_name
    assert "item.localized_name" in item_name


if __name__ == "__main__":
    test_interact_key_uses_full_key_parser_not_first_character()
    test_config_ui_saves_normalized_interact_key_names()
    test_language_setting_exists_in_defaults_and_ui()
    test_ui_item_and_category_text_can_switch_to_english()
    print("config language and key tests passed")
