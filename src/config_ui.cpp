#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cwctype>
#include <cstring>
#include <map>
#include <string>
#include <vector>

namespace {

constexpr int kWindowWidth = 620;
constexpr int kWindowHeight = 650;
constexpr COLORREF kColorBg = RGB(18, 18, 16);
constexpr COLORREF kColorPanel = RGB(34, 32, 27);
constexpr COLORREF kColorPanelAlt = RGB(45, 41, 33);
constexpr COLORREF kColorEdit = RGB(24, 23, 20);
constexpr COLORREF kColorAccent = RGB(191, 142, 73);
constexpr COLORREF kColorSelect = RGB(97, 66, 32);
constexpr COLORREF kColorText = RGB(231, 224, 205);
constexpr COLORREF kColorMuted = RGB(171, 157, 130);

enum ControlId {
  IDC_ENABLED = 1001,
  IDC_GROUND,
  IDC_CORPSE,
  IDC_FILTER,
  IDC_DEBUG,
  IDC_FOREGROUND,
  IDC_KEY_EDIT,
  IDC_INTERVAL_EDIT,
  IDC_TOGGLE_HOTKEY_EDIT,
  IDC_CONFIG_HOTKEY_EDIT,
  IDC_LANGUAGE_COMBO,
  IDC_SAVE,
  IDC_RELOAD,
  IDC_OPEN_LOG,
  IDC_OPEN_ITEMS,
  IDC_STATUS,
  IDC_ITEM_LIST,
  IDC_CATEGORY_LIST,
  IDC_SEARCH_EDIT,
  IDC_CLEAR_SEARCH,
  IDC_BLOCK_ITEM,
  IDC_UNBLOCK_ITEM,
  IDC_CAT_UNKNOWN = 1200,
  IDC_CAT_CURRENCY,
  IDC_CAT_MATERIAL,
  IDC_CAT_CONSUMABLE,
  IDC_CAT_FOOD,
  IDC_CAT_RECIPE,
  IDC_CAT_DOCUMENT,
  IDC_CAT_TRADE,
  IDC_CAT_AMMO,
  IDC_CAT_QUEST,
  IDC_CAT_ONE_HAND_WEAPON,
  IDC_CAT_TWO_HAND_WEAPON,
  IDC_CAT_BOW,
  IDC_CAT_SHIELD,
  IDC_CAT_TOWER_SHIELD,
  IDC_CAT_HELMET,
  IDC_CAT_CHEST_ARMOR,
  IDC_CAT_GLOVES,
  IDC_CAT_BOOTS,
  IDC_CAT_CLOAK,
  IDC_CAT_RING,
  IDC_CAT_NECKLACE,
  IDC_CAT_EARRING,
  IDC_CAT_BRACELET,
  IDC_CAT_HEAD_ACCESSORY,
  IDC_CAT_FACE_ACCESSORY,
  IDC_CAT_TOOL,
  IDC_CAT_HORSE_GEAR,
  IDC_CAT_PET_GEAR,
  IDC_CAT_VEHICLE_GEAR,
  IDC_CAT_BACKPACK,
  IDC_CAT_ABYSS_GEAR,
  IDC_CAT_MISC,
  IDC_LABEL_TITLE = 1400,
  IDC_LABEL_INTERACT_KEY,
  IDC_LABEL_INTERVAL,
  IDC_LABEL_TOGGLE_HOTKEY,
  IDC_LABEL_CONFIG_HOTKEY,
  IDC_LABEL_LANGUAGE,
  IDC_LABEL_CATEGORY,
  IDC_LABEL_ITEMS,
  IDC_LABEL_SEARCH,
};

struct CategoryControl {
  int id;
  const wchar_t* key;
  const wchar_t* label;
  const wchar_t* english_label;
};

struct ItemRow {
  uint32_t key = 0;
  std::wstring category;
  std::wstring localized_name;
  std::wstring english_name;
  std::wstring internal_name;
  bool blocked = false;
};

const CategoryControl kCategories[] = {
    {IDC_CAT_UNKNOWN, L"Unknown", L"\u672a\u77e5/\u672a\u89e3\u6790", L"Unknown"},
    {IDC_CAT_CURRENCY, L"Currency", L"\u8d27\u5e01", L"Currency"},
    {IDC_CAT_MATERIAL, L"Material", L"\u6750\u6599", L"Material"},
    {IDC_CAT_CONSUMABLE, L"Consumable", L"\u6d88\u8017\u54c1", L"Consumable"},
    {IDC_CAT_FOOD, L"Food", L"\u98df\u7269/\u9c7c\u7c7b", L"Food / Fish"},
    {IDC_CAT_RECIPE, L"Recipe", L"\u56fe\u7eb8/\u914d\u65b9", L"Recipe"},
    {IDC_CAT_DOCUMENT, L"Document", L"\u4e66\u7c4d/\u6587\u4ef6", L"Document"},
    {IDC_CAT_TRADE, L"Trade", L"\u8d38\u6613\u54c1", L"Trade"},
    {IDC_CAT_AMMO, L"Ammo", L"\u5f39\u836f", L"Ammo"},
    {IDC_CAT_QUEST, L"Quest", L"\u4efb\u52a1\u7269\u54c1", L"Quest"},
    {IDC_CAT_ONE_HAND_WEAPON, L"OneHandWeapon", L"\u5355\u624b\u6b66\u5668", L"One-Hand Weapons"},
    {IDC_CAT_TWO_HAND_WEAPON, L"TwoHandWeapon", L"\u53cc\u624b\u6b66\u5668", L"Two-Hand Weapons"},
    {IDC_CAT_BOW, L"Bow", L"\u5f13/\u5f29", L"Bows / Crossbows"},
    {IDC_CAT_SHIELD, L"Shield", L"\u76fe\u724c", L"Shields"},
    {IDC_CAT_TOWER_SHIELD, L"TowerShield", L"\u5927\u578b\u76fe\u724c", L"Tower Shields"},
    {IDC_CAT_HELMET, L"Helmet", L"\u5934\u76d4", L"Helmets"},
    {IDC_CAT_CHEST_ARMOR, L"ChestArmor", L"\u80f8\u7532/\u670d\u88c5", L"Chest Armor"},
    {IDC_CAT_GLOVES, L"Gloves", L"\u624b\u5957", L"Gloves"},
    {IDC_CAT_BOOTS, L"Boots", L"\u978b\u5b50", L"Boots"},
    {IDC_CAT_CLOAK, L"Cloak", L"\u62ab\u98ce", L"Cloaks"},
    {IDC_CAT_RING, L"Ring", L"\u6212\u6307", L"Rings"},
    {IDC_CAT_NECKLACE, L"Necklace", L"\u9879\u94fe", L"Necklaces"},
    {IDC_CAT_EARRING, L"Earring", L"\u8033\u73af", L"Earrings"},
    {IDC_CAT_BRACELET, L"Bracelet", L"\u624b\u956f", L"Bracelets"},
    {IDC_CAT_HEAD_ACCESSORY, L"HeadAccessory", L"\u5934\u9970", L"Head Accessories"},
    {IDC_CAT_FACE_ACCESSORY, L"FaceAccessory", L"\u9762\u9970", L"Face Accessories"},
    {IDC_CAT_TOOL, L"Tool", L"\u5de5\u5177", L"Tool"},
    {IDC_CAT_HORSE_GEAR, L"HorseGear", L"\u9a6c\u5177", L"Horse Gear"},
    {IDC_CAT_PET_GEAR, L"PetGear", L"\u5ba0\u7269\u88c5\u5907", L"Pet Gear"},
    {IDC_CAT_VEHICLE_GEAR, L"VehicleGear", L"\u8f7d\u5177\u88c5\u5907", L"Vehicle Gear"},
    {IDC_CAT_BACKPACK, L"Backpack", L"\u80cc\u56ca", L"Backpacks"},
    {IDC_CAT_ABYSS_GEAR, L"AbyssGear", L"\u963f\u6bd4\u65af\u88c5\u5907", L"Abyss Gear"},
    {IDC_CAT_MISC, L"Misc", L"\u6742\u9879", L"Misc"},
};
constexpr size_t kCategoryCount = sizeof(kCategories) / sizeof(kCategories[0]);

enum class UiLanguage {
  kChinese,
  kEnglish,
};

HINSTANCE g_instance = nullptr;
HWND g_hwnd = nullptr;
HFONT g_font = nullptr;
HBRUSH g_bg_brush = nullptr;
HBRUSH g_panel_brush = nullptr;
HBRUSH g_edit_brush = nullptr;
HANDLE g_single_instance = nullptr;
WNDPROC g_category_list_proc = nullptr;
WNDPROC g_item_list_proc = nullptr;
std::wstring g_dir;
std::wstring g_ini_path;
std::wstring g_default_ini_path;
std::wstring g_log_path;
std::wstring g_items_path;
std::map<std::wstring, int> g_category_counts;
std::vector<ItemRow> g_items;
std::vector<size_t> g_displayed_item_indices;
std::wstring g_item_language;
std::array<bool, kCategoryCount> g_category_enabled_ui{};
std::wstring g_language_setting = L"Auto";
UiLanguage g_ui_language = UiLanguage::kChinese;

std::wstring ModuleDirectory() {
  wchar_t path[MAX_PATH]{};
  GetModuleFileNameW(nullptr, path, MAX_PATH);
  std::wstring result(path);
  const size_t slash = result.find_last_of(L"\\/");
  if (slash != std::wstring::npos) result.resize(slash);
  return result;
}

std::wstring TrimIniWide(const std::wstring& text) {
  size_t begin = 0;
  size_t end = text.size();
  while (begin < end && std::iswspace(text[begin])) ++begin;
  while (end > begin && std::iswspace(text[end - 1])) --end;
  return text.substr(begin, end - begin);
}

std::wstring LoadIniTextManual() {
  FILE* f = nullptr;
  if (_wfopen_s(&f, g_ini_path.c_str(), L"rb") != 0 || !f) return {};
  std::fseek(f, 0, SEEK_END);
  const long size_long = std::ftell(f);
  std::fseek(f, 0, SEEK_SET);
  if (size_long <= 0) {
    std::fclose(f);
    return {};
  }

  std::vector<unsigned char> bytes(static_cast<size_t>(size_long));
  const size_t read = std::fread(bytes.data(), 1, bytes.size(), f);
  std::fclose(f);
  bytes.resize(read);
  if (bytes.empty()) return {};

  if (bytes.size() >= 2 && bytes[0] == 0xFF && bytes[1] == 0xFE) {
    const size_t chars = (bytes.size() - 2) / sizeof(wchar_t);
    return std::wstring(reinterpret_cast<const wchar_t*>(bytes.data() + 2),
                        chars);
  }

  size_t offset = 0;
  if (bytes.size() >= 3 && bytes[0] == 0xEF && bytes[1] == 0xBB &&
      bytes[2] == 0xBF) {
    offset = 3;
  }
  const char* data = reinterpret_cast<const char*>(bytes.data() + offset);
  const int len = static_cast<int>(bytes.size() - offset);
  int needed = MultiByteToWideChar(CP_UTF8, 0, data, len, nullptr, 0);
  UINT codepage = CP_UTF8;
  if (needed <= 0) {
    codepage = CP_ACP;
    needed = MultiByteToWideChar(codepage, 0, data, len, nullptr, 0);
  }
  if (needed <= 0) return {};
  std::wstring out(static_cast<size_t>(needed), L'\0');
  MultiByteToWideChar(codepage, 0, data, len, out.data(), needed);
  return out;
}

bool ReadIniStringManual(const wchar_t* section, const wchar_t* key,
                         std::wstring* value) {
  if (!section || !key || !value) return false;
  const std::wstring text = LoadIniTextManual();
  if (text.empty()) return false;

  std::wstring current_section;
  size_t pos = 0;
  bool first_line = true;
  while (pos <= text.size()) {
    size_t next = text.find(L'\n', pos);
    std::wstring line =
        next == std::wstring::npos ? text.substr(pos) : text.substr(pos, next - pos);
    if (!line.empty() && line.back() == L'\r') line.pop_back();
    if (first_line && !line.empty() && line[0] == 0xFEFF) line.erase(0, 1);
    first_line = false;

    line = TrimIniWide(line);
    if (!line.empty() && line[0] != L';' && line[0] != L'#') {
      if (line.front() == L'[') {
        const size_t close = line.find(L']');
        if (close != std::wstring::npos) {
          current_section = TrimIniWide(line.substr(1, close - 1));
        }
      } else if (_wcsicmp(current_section.c_str(), section) == 0) {
        const size_t eq = line.find(L'=');
        if (eq != std::wstring::npos) {
          std::wstring name = TrimIniWide(line.substr(0, eq));
          if (_wcsicmp(name.c_str(), key) == 0) {
            *value = TrimIniWide(line.substr(eq + 1));
            return true;
          }
        }
      }
    }

    if (next == std::wstring::npos) break;
    pos = next + 1;
  }
  return false;
}

std::wstring ReadIniString(const wchar_t* section, const wchar_t* key,
                           const wchar_t* fallback) {
  wchar_t buffer[128]{};
  GetPrivateProfileStringW(section, key, L"", buffer,
                           static_cast<DWORD>(sizeof(buffer) / sizeof(buffer[0])),
                           g_ini_path.c_str());
  if (buffer[0] != L'\0') return buffer;
  std::wstring manual;
  if (ReadIniStringManual(section, key, &manual)) return manual;
  if (fallback && fallback[0]) return fallback;
  return buffer;
}

int ReadIniInt(const wchar_t* section, const wchar_t* key, int fallback) {
  const std::wstring value = ReadIniString(section, key, L"");
  if (value.empty()) return fallback;
  wchar_t* end = nullptr;
  const long parsed = std::wcstol(value.c_str(), &end, 0);
  return end != value.c_str() ? static_cast<int>(parsed) : fallback;
}

void WriteIniInt(const wchar_t* section, const wchar_t* key, int value) {
  WritePrivateProfileStringW(section, key, value ? L"1" : L"0",
                             g_ini_path.c_str());
}

void WriteIniString(const wchar_t* section, const wchar_t* key,
                    const std::wstring& value) {
  WritePrivateProfileStringW(section, key, value.c_str(), g_ini_path.c_str());
}

bool IsChineseSystemLanguage() {
  const LANGID lang = GetUserDefaultUILanguage();
  return PRIMARYLANGID(lang) == LANG_CHINESE;
}

std::wstring NormalizeLanguageSetting(std::wstring value) {
  for (wchar_t& ch : value) {
    if (ch >= L'a' && ch <= L'z') ch = static_cast<wchar_t>(ch - L'a' + L'A');
  }
  if (value == L"ZH" || value == L"CHINESE" || value == L"CN") return L"zh";
  if (value == L"EN" || value == L"ENGLISH") return L"en";
  return L"Auto";
}

UiLanguage ResolveLanguage(const std::wstring& setting) {
  const std::wstring normalized = NormalizeLanguageSetting(setting);
  if (normalized == L"zh") return UiLanguage::kChinese;
  if (normalized == L"en") return UiLanguage::kEnglish;
  return IsChineseSystemLanguage() ? UiLanguage::kChinese
                                   : UiLanguage::kEnglish;
}

void LoadLanguageSetting() {
  g_language_setting =
      NormalizeLanguageSetting(ReadIniString(L"General", L"Language", L"Auto"));
  g_ui_language = ResolveLanguage(g_language_setting);
}

bool IsEnglishUi() { return g_ui_language == UiLanguage::kEnglish; }

enum class TextId {
  kWindowTitle,
  kTitle,
  kEnabled,
  kGround,
  kCorpse,
  kFilter,
  kForeground,
  kDebug,
  kInteractKey,
  kInterval,
  kToggleHotkey,
  kConfigHotkey,
  kLanguage,
  kCategory,
  kItems,
  kSearch,
  kClear,
  kSave,
  kReload,
  kLog,
  kItemTable,
  kAutoLanguage,
  kChineseLanguage,
  kEnglishLanguage,
  kStatusChooseCategory,
  kStatusCurrentCategory,
  kStatusShown,
  kStatusRows,
  kStatusSaved,
  kStatusHotkeyConflict,
  kStatusSelectItem,
  kStatusBlocked,
  kStatusRestored,
  kStatusCategoryEnabled,
  kStatusCategoryDisabled,
};

const wchar_t* UiText(TextId id) {
  const bool en = IsEnglishUi();
  switch (id) {
    case TextId::kWindowTitle:
      return en ? L"Crimson Desert AutoLoot CN Config"
                : L"\u81ea\u52a8\u62fe\u53d6\u4e2d\u6587\u914d\u7f6e";
    case TextId::kTitle:
      return en ? L"Crimson Desert AutoLoot" : L"Crimson Desert \u81ea\u52a8\u62fe\u53d6";
    case TextId::kEnabled:
      return en ? L"Enable" : L"\u542f\u7528";
    case TextId::kGround:
      return en ? L"Ground" : L"\u5730\u9762";
    case TextId::kCorpse:
      return en ? L"Corpse" : L"\u6478\u5c38";
    case TextId::kFilter:
      return en ? L"Filter" : L"\u8fc7\u6ee4";
    case TextId::kForeground:
      return en ? L"Only when game is foreground"
                : L"\u53ea\u5728\u6e38\u620f\u524d\u53f0\u65f6\u89e6\u53d1";
    case TextId::kDebug:
      return en ? L"Debug" : L"\u8c03\u8bd5";
    case TextId::kInteractKey:
      return en ? L"Interact" : L"\u4ea4\u4e92\u952e";
    case TextId::kInterval:
      return en ? L"Interval" : L"\u95f4\u9694";
    case TextId::kToggleHotkey:
      return en ? L"Toggle" : L"\u5f00\u5173\u952e";
    case TextId::kConfigHotkey:
      return en ? L"Panel" : L"\u9762\u677f\u952e";
    case TextId::kLanguage:
      return en ? L"Language" : L"\u8bed\u8a00";
    case TextId::kCategory:
      return en ? L"Category" : L"\u5206\u7c7b";
    case TextId::kItems:
      return en ? L"Items" : L"\u7269\u54c1";
    case TextId::kSearch:
      return en ? L"Search" : L"\u641c\u7d22";
    case TextId::kClear:
      return en ? L"Clear" : L"\u6e05\u7a7a";
    case TextId::kSave:
      return en ? L"Save" : L"\u4fdd\u5b58";
    case TextId::kReload:
      return en ? L"Reload" : L"\u91cd\u8bfb";
    case TextId::kLog:
      return en ? L"Log" : L"\u65e5\u5fd7";
    case TextId::kItemTable:
      return en ? L"Item Table" : L"\u7269\u54c1\u8868";
    case TextId::kAutoLanguage:
      return en ? L"Auto" : L"\u81ea\u52a8";
    case TextId::kChineseLanguage:
      return en ? L"Chinese" : L"\u4e2d\u6587";
    case TextId::kEnglishLanguage:
      return en ? L"English" : L"\u82f1\u6587";
    case TextId::kStatusChooseCategory:
      return en ? L"Select a category on the left to show items."
                : L"\u8bf7\u5728\u5de6\u4fa7\u9009\u62e9\u4e00\u4e2a\u7c7b\u522b\u67e5\u770b\u7269\u54c1\u3002";
    case TextId::kStatusCurrentCategory:
      return en ? L"Current category " : L"\u5f53\u524d\u7c7b\u522b ";
    case TextId::kStatusShown:
      return en ? L", showing " : L"\uff0c\u663e\u793a ";
    case TextId::kStatusRows:
      return en ? L" rows." : L" \u6761\u3002";
    case TextId::kStatusSaved:
      return en ? L"Saved. The in-game plugin hot-reloads automatically."
                : L"\u5df2\u4fdd\u5b58\u3002\u6e38\u620f\u5185\u63d2\u4ef6\u4f1a\u81ea\u52a8\u70ed\u52a0\u8f7d\uff0c\u65e0\u9700\u91cd\u542f\u3002";
    case TextId::kStatusHotkeyConflict:
      return en ? L"Saved. Conflicting hotkeys were reset to F9 / F10."
                : L"\u5df2\u4fdd\u5b58\u3002\u70ed\u952e\u4e0e\u6e38\u620f\u6309\u952e\u51b2\u7a81\uff0c\u5df2\u81ea\u52a8\u6539\u4e3a F9 / F10\u3002";
    case TextId::kStatusSelectItem:
      return en ? L"Select an item in the item list first."
                : L"\u8bf7\u5148\u5728\u7269\u54c1\u5217\u8868\u91cc\u9009\u4e2d\u4e00\u4e2a\u7269\u54c1\u3002";
    case TextId::kStatusBlocked:
      return en ? L"Disabled pickup: " : L"\u5df2\u8bbe\u4e3a\u4e0d\u62fe\u53d6\uff1a";
    case TextId::kStatusRestored:
      return en ? L"Enabled pickup: " : L"\u5df2\u6062\u590d\u62fe\u53d6\uff1a";
    case TextId::kStatusCategoryEnabled:
      return en ? L"Enabled category pickup and restored all items: "
                : L"\u5df2\u5f00\u542f\u5206\u7c7b\u62fe\u53d6\u5e76\u6062\u590d\u8be5\u7c7b\u5168\u90e8\u7269\u54c1\uff1a";
    case TextId::kStatusCategoryDisabled:
      return en ? L"Disabled category pickup and blocked all items: "
                : L"\u5df2\u5173\u95ed\u5206\u7c7b\u62fe\u53d6\u5e76\u53d6\u6d88\u8be5\u7c7b\u5168\u90e8\u7269\u54c1\uff1a";
  }
  return L"";
}

void RefreshCategoryList();
void RefreshItemList();
void ApplyLanguageToUi();

HWND Ctrl(int id) { return GetDlgItem(g_hwnd, id); }

void SetCheck(int id, bool checked) {
  SendMessageW(Ctrl(id), BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED, 0);
}

bool GetCheck(int id) {
  return SendMessageW(Ctrl(id), BM_GETCHECK, 0, 0) == BST_CHECKED;
}

std::wstring GetText(int id) {
  wchar_t buffer[256]{};
  HWND hwnd = Ctrl(id);
  if (!hwnd) return L"";
  GetWindowTextW(hwnd, buffer, static_cast<int>(sizeof(buffer) / sizeof(buffer[0])));
  return buffer;
}

void SetText(int id, const std::wstring& text) {
  HWND hwnd = Ctrl(id);
  if (hwnd) SetWindowTextW(hwnd, text.c_str());
}

void SetStatus(const std::wstring& text) { SetText(IDC_STATUS, text); }

int LanguageComboIndexFromSetting(const std::wstring& setting) {
  const std::wstring normalized = NormalizeLanguageSetting(setting);
  if (normalized == L"zh") return 1;
  if (normalized == L"en") return 2;
  return 0;
}

std::wstring LanguageSettingFromComboIndex(int index) {
  if (index == 1) return L"zh";
  if (index == 2) return L"en";
  return L"Auto";
}

void FillLanguageCombo() {
  HWND combo = Ctrl(IDC_LANGUAGE_COMBO);
  if (!combo) return;
  SendMessageW(combo, CB_RESETCONTENT, 0, 0);
  SendMessageW(combo, CB_ADDSTRING, 0,
               reinterpret_cast<LPARAM>(UiText(TextId::kAutoLanguage)));
  SendMessageW(combo, CB_ADDSTRING, 0,
               reinterpret_cast<LPARAM>(UiText(TextId::kChineseLanguage)));
  SendMessageW(combo, CB_ADDSTRING, 0,
               reinterpret_cast<LPARAM>(UiText(TextId::kEnglishLanguage)));
  SendMessageW(combo, CB_SETCURSEL,
               static_cast<WPARAM>(LanguageComboIndexFromSetting(g_language_setting)),
               0);
}

void SaveLanguageFromCombo() {
  HWND combo = Ctrl(IDC_LANGUAGE_COMBO);
  if (!combo) return;
  const int index = static_cast<int>(SendMessageW(combo, CB_GETCURSEL, 0, 0));
  g_language_setting = LanguageSettingFromComboIndex(index);
  g_ui_language = ResolveLanguage(g_language_setting);
  WriteIniString(L"General", L"Language", g_language_setting);
}

std::wstring Utf8ToWide(const char* text) {
  if (!text) return L"";
  const int needed = MultiByteToWideChar(CP_UTF8, 0, text, -1, nullptr, 0);
  if (needed <= 1) return L"";
  std::wstring result(static_cast<size_t>(needed - 1), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, text, -1, result.data(), needed);
  return result;
}

std::vector<std::string> SplitTabs(char* line) {
  std::vector<std::string> result;
  char* cursor = line;
  while (cursor) {
    char* tab = std::strchr(cursor, '\t');
    if (tab) *tab = 0;
    char* end = cursor + std::strlen(cursor);
    while (end > cursor && (end[-1] == '\r' || end[-1] == '\n')) {
      *--end = 0;
    }
    result.emplace_back(cursor);
    cursor = tab ? tab + 1 : nullptr;
  }
  return result;
}

const CategoryControl* FindCategory(const std::wstring& key) {
  for (const auto& category : kCategories) {
    if (key == category.key) return &category;
  }
  return nullptr;
}

int CategoryIndexByKey(const std::wstring& key) {
  for (size_t i = 0; i < kCategoryCount; ++i) {
    if (key == kCategories[i].key) return static_cast<int>(i);
  }
  return -1;
}

std::wstring CategoryDisplay(const std::wstring& key) {
  const CategoryControl* category = FindCategory(key);
  if (!category) return key;
  return IsEnglishUi() ? category->english_label : category->label;
}

bool CategoryDefaultChecked(const wchar_t* key) {
  if (!key) return true;
  return lstrcmpW(key, L"Unknown") != 0 &&
         lstrcmpW(key, L"Consumable") != 0 &&
         lstrcmpW(key, L"Food") != 0 &&
         lstrcmpW(key, L"Recipe") != 0 &&
         lstrcmpW(key, L"Document") != 0 &&
         lstrcmpW(key, L"Trade") != 0 &&
         lstrcmpW(key, L"Ammo") != 0 &&
         lstrcmpW(key, L"Quest") != 0 &&
         lstrcmpW(key, L"HorseGear") != 0 &&
         lstrcmpW(key, L"PetGear") != 0 &&
         lstrcmpW(key, L"VehicleGear") != 0 &&
         lstrcmpW(key, L"Backpack") != 0 &&
         lstrcmpW(key, L"Misc") != 0 &&
         lstrcmpW(key, L"Equipment") != 0;
}

std::wstring ToLower(std::wstring text) {
  for (wchar_t& ch : text) ch = static_cast<wchar_t>(std::towlower(ch));
  return text;
}

std::wstring Trim(std::wstring text) {
  while (!text.empty() &&
         (text.front() == L' ' || text.front() == L'\t' ||
          text.front() == L'\r' || text.front() == L'\n')) {
    text.erase(text.begin());
  }
  while (!text.empty() &&
         (text.back() == L' ' || text.back() == L'\t' ||
          text.back() == L'\r' || text.back() == L'\n')) {
    text.pop_back();
  }
  return text;
}

std::wstring UpperAscii(std::wstring text) {
  for (wchar_t& ch : text) {
    if (ch >= L'a' && ch <= L'z') ch = static_cast<wchar_t>(ch - L'a' + L'A');
  }
  return text;
}

bool IsValidKeyName(const std::wstring& token) {
  if (token.size() == 1) {
    const wchar_t ch = token[0];
    return (ch >= L'A' && ch <= L'Z') || (ch >= L'0' && ch <= L'9') ||
           ch == L'`';
  }
  if (token.size() >= 2 && token[0] == L'F') {
    const int fn = _wtoi(token.c_str() + 1);
    return fn >= 1 && fn <= 24;
  }
  return token == L"SPACE" || token == L"SPACEBAR" || token == L"TAB" ||
         token == L"ENTER" || token == L"RETURN" || token == L"ESC" ||
         token == L"ESCAPE" || token == L"BACKSPACE" ||
         token == L"BKSP" || token == L"INS" || token == L"INSERT" ||
         token == L"DEL" || token == L"DELETE" || token == L"HOME" ||
         token == L"END" || token == L"PGUP" || token == L"PAGEUP" ||
         token == L"PGDN" || token == L"PAGEDOWN" || token == L"UP" ||
         token == L"DOWN" || token == L"LEFT" || token == L"RIGHT" ||
         token == L"CAPS" || token == L"CAPSLOCK" || token == L"SHIFT" ||
         token == L"CTRL" || token == L"CONTROL" || token == L"ALT" ||
         token == L"MENU";
}

std::wstring NormalizeInteractKeyText(const std::wstring& text,
                                      const wchar_t* fallback) {
  std::wstring key = UpperAscii(Trim(text));
  std::replace(key.begin(), key.end(), L'-', L'+');
  const size_t plus = key.find_last_of(L'+');
  if (plus != std::wstring::npos) key = Trim(key.substr(plus + 1));
  return IsValidKeyName(key) ? key : fallback;
}

std::wstring NormalizeHotkeyText(const std::wstring& text,
                                 const wchar_t* fallback) {
  std::wstring source = UpperAscii(text);
  std::replace(source.begin(), source.end(), L'-', L'+');

  bool alt = false;
  bool ctrl = false;
  bool shift = false;
  std::wstring key;
  size_t start = 0;
  while (start <= source.size()) {
    const size_t plus = source.find(L'+', start);
    std::wstring token = Trim(source.substr(
        start, plus == std::wstring::npos ? std::wstring::npos : plus - start));
    if (!token.empty()) {
      if (token == L"ALT" || token == L"MENU") {
        alt = true;
      } else if (token == L"CTRL" || token == L"CONTROL") {
        ctrl = true;
      } else if (token == L"SHIFT") {
        shift = true;
      } else if (IsValidKeyName(token)) {
        key = token;
      }
    }
    if (plus == std::wstring::npos) break;
    start = plus + 1;
  }

  if (key.empty()) return fallback;
  std::wstring result;
  if (ctrl) result += L"Ctrl+";
  if (alt) result += L"Alt+";
  if (shift) result += L"Shift+";
  result += key;
  return result;
}

std::wstring HotkeyMainKey(std::wstring hotkey) {
  hotkey = UpperAscii(Trim(hotkey));
  const size_t plus = hotkey.find_last_of(L'+');
  if (plus == std::wstring::npos) return hotkey;
  return Trim(hotkey.substr(plus + 1));
}

bool IsGameReservedHotkey(const std::wstring& hotkey) {
  const std::wstring key = HotkeyMainKey(hotkey);
  if (key.size() == 1) {
    switch (key[0]) {
      case L'W':
      case L'A':
      case L'S':
      case L'D':
      case L'Q':
      case L'E':
      case L'R':
      case L'T':
      case L'I':
      case L'J':
      case L'K':
      case L'M':
      case L'B':
      case L'V':
      case L'X':
      case L'C':
      case L'Z':
      case L'F':
      case L'H':
      case L'`':
        return true;
      default:
        return false;
    }
  }
  return key == L"F1" || key == L"F2" || key == L"F3" || key == L"TAB" ||
         key == L"CAPS" || key == L"CAPSLOCK" || key == L"SHIFT" ||
         key == L"CTRL" || key == L"CONTROL" || key == L"ALT" ||
         key == L"MENU" || key == L"SPACE";
}

std::wstring SafeHotkeyText(const std::wstring& input, const wchar_t* fallback,
                            bool* conflicted) {
  std::wstring hotkey = NormalizeHotkeyText(input, fallback);
  if (IsGameReservedHotkey(hotkey)) {
    if (conflicted) *conflicted = true;
    hotkey = fallback;
  }
  return hotkey;
}

bool ContainsInsensitive(const std::wstring& haystack,
                         const std::wstring& needle_lower) {
  if (needle_lower.empty()) return true;
  return ToLower(haystack).find(needle_lower) != std::wstring::npos;
}

std::wstring ItemKeyString(uint32_t key) {
  return std::to_wstring(key);
}

bool ReadItemBlocked(uint32_t key) {
  const std::wstring key_text = ItemKeyString(key);
  return ReadIniInt(L"BlockedItems", key_text.c_str(), 0) != 0;
}

void WriteItemBlocked(uint32_t key, bool blocked) {
  const std::wstring key_text = ItemKeyString(key);
  WritePrivateProfileStringW(L"BlockedItems", key_text.c_str(),
                             blocked ? L"1" : nullptr, g_ini_path.c_str());
}

void WriteBlockedItemsSection() {
  std::map<uint32_t, bool> blocked_keys;
  for (const ItemRow& item : g_items) {
    if (item.blocked) {
      blocked_keys[item.key] = true;
    }
  }

  std::wstring packed;
  for (const auto& entry : blocked_keys) {
    packed += ItemKeyString(entry.first);
    packed += L"=1";
    packed.push_back(L'\0');
  }
  packed.push_back(L'\0');

  WritePrivateProfileSectionW(L"BlockedItems", packed.c_str(),
                              g_ini_path.c_str());
}

void SetItemBlockedInMemory(ItemRow& item, bool blocked) {
  item.blocked = blocked;
}

void SetItemBlocked(ItemRow& item, bool blocked) {
  SetItemBlockedInMemory(item, blocked);
  WriteItemBlocked(item.key, blocked);
}

void SetAllItemsInCategoryBlocked(const std::wstring& category, bool blocked) {
  bool changed = false;
  for (ItemRow& item : g_items) {
    if (item.category == category) {
      changed = changed || item.blocked != blocked;
      SetItemBlockedInMemory(item, blocked);
    }
  }
  if (changed) {
    WriteBlockedItemsSection();
  }
}

void LoadItems() {
  g_category_counts.clear();
  g_items.clear();
  g_item_language.clear();

  FILE* f = nullptr;
  _wfopen_s(&f, g_items_path.c_str(), L"rb");
  if (!f) return;

  char line[4096]{};
  while (std::fgets(line, sizeof(line), f)) {
    if (line[0] == '#') {
      if (std::strncmp(line, "# locale=", 9) == 0) {
        g_item_language = Utf8ToWide(line + 2);
      }
      continue;
    }

    std::vector<std::string> fields = SplitTabs(line);
    if (fields.size() < 5) continue;

    ItemRow item{};
    item.key = static_cast<uint32_t>(std::strtoul(fields[0].c_str(), nullptr, 10));
    item.category = Utf8ToWide(fields[1].c_str());
    item.localized_name = Utf8ToWide(fields[2].c_str());
    item.english_name = Utf8ToWide(fields[3].c_str());
    item.internal_name = Utf8ToWide(fields[4].c_str());
    if (item.key == 0 || item.category.empty()) continue;
    item.blocked = ReadItemBlocked(item.key);

    g_category_counts[item.category]++;
    g_items.push_back(std::move(item));
  }
  std::fclose(f);
}

std::wstring CategoryLabel(const CategoryControl& category) {
  wchar_t buffer[256]{};
  const auto it = g_category_counts.find(category.key);
  const int count = it == g_category_counts.end() ? 0 : it->second;
  const std::wstring label = CategoryDisplay(category.key);
  swprintf_s(buffer, L"%s  (%d)", label.c_str(), count);
  return buffer;
}

std::wstring CurrentLanguageItemName(const ItemRow& item) {
  if (IsEnglishUi() && !item.english_name.empty()) return item.english_name;
  if (!item.localized_name.empty()) return item.localized_name;
  if (!item.english_name.empty()) return item.english_name;
  if (!item.internal_name.empty()) return item.internal_name;
  return L"#" + std::to_wstring(item.key);
}

std::wstring BuildItemText(const ItemRow& item) {
  return CurrentLanguageItemName(item);
}

void EnsureIniDefaults() {
  if (GetFileAttributesW(g_ini_path.c_str()) != INVALID_FILE_ATTRIBUTES) {
    return;
  }
  if (GetFileAttributesW(g_default_ini_path.c_str()) != INVALID_FILE_ATTRIBUTES &&
      CopyFileW(g_default_ini_path.c_str(), g_ini_path.c_str(), FALSE)) {
    return;
  }
  WritePrivateProfileStringW(L"General", L"Enabled", L"1", g_ini_path.c_str());
  WritePrivateProfileStringW(L"General", L"DebugLog", L"1", g_ini_path.c_str());
  WritePrivateProfileStringW(L"General", L"ForegroundOnly", L"1",
                             g_ini_path.c_str());
  WritePrivateProfileStringW(L"General", L"TriggerIntervalMs", L"650",
                             g_ini_path.c_str());
  WritePrivateProfileStringW(L"General", L"InteractKey", L"E",
                             g_ini_path.c_str());
  WritePrivateProfileStringW(L"General", L"ToggleHotkey", L"F9",
                             g_ini_path.c_str());
  WritePrivateProfileStringW(L"General", L"ConfigHotkey", L"F10",
                             g_ini_path.c_str());
  WritePrivateProfileStringW(L"General", L"Language", L"Auto",
                             g_ini_path.c_str());
  WritePrivateProfileStringW(L"General", L"StrictVersionCheck", L"0",
                             g_ini_path.c_str());
  WritePrivateProfileStringW(L"Features", L"GroundLoot", L"1",
                             g_ini_path.c_str());
  WritePrivateProfileStringW(L"Features", L"CorpseLoot", L"1",
                             g_ini_path.c_str());
  WritePrivateProfileStringW(L"ItemFilter", L"Enabled", L"1",
                             g_ini_path.c_str());
  for (const auto& category : kCategories) {
    WritePrivateProfileStringW(
        L"ItemFilter", category.key,
        CategoryDefaultChecked(category.key) ? L"1" : L"0", g_ini_path.c_str());
  }
}

bool CategoryChecked(const std::wstring& key) {
  const int index = CategoryIndexByKey(key);
  if (index < 0) return true;
  return g_category_enabled_ui[static_cast<size_t>(index)];
}

void RefreshCategoryList() {
  HWND list = Ctrl(IDC_CATEGORY_LIST);
  SendMessageW(list, LB_RESETCONTENT, 0, 0);
  for (const auto& category : kCategories) {
    SendMessageW(list, LB_ADDSTRING, 0,
                 reinterpret_cast<LPARAM>(CategoryLabel(category).c_str()));
  }
  SendMessageW(list, LB_SETCURSEL, static_cast<WPARAM>(-1), 0);
}

int SelectedCategoryIndex() {
  const LRESULT sel = SendMessageW(Ctrl(IDC_CATEGORY_LIST), LB_GETCURSEL, 0, 0);
  if (sel == LB_ERR) return -1;
  if (sel < 0 ||
      sel >= static_cast<LRESULT>(sizeof(kCategories) / sizeof(kCategories[0]))) {
    return -1;
  }
  return static_cast<int>(sel);
}

std::wstring SelectedCategoryKey() {
  const int index = SelectedCategoryIndex();
  if (index < 0) return L"";
  return kCategories[index].key;
}

void RefreshItemList() {
  HWND list = Ctrl(IDC_ITEM_LIST);
  SendMessageW(list, WM_SETREDRAW, FALSE, 0);
  SendMessageW(list, LB_RESETCONTENT, 0, 0);
  g_displayed_item_indices.clear();

  int shown = 0;
  const std::wstring selected_category = SelectedCategoryKey();
  const std::wstring search = ToLower(GetText(IDC_SEARCH_EDIT));
  if (!selected_category.empty()) {
    for (size_t index = 0; index < g_items.size(); ++index) {
      const ItemRow& item = g_items[index];
      if (item.category != selected_category) continue;

      const std::wstring display_name = CurrentLanguageItemName(item);
      if (!search.empty() && !ContainsInsensitive(display_name, search)) {
        continue;
      }

      std::wstring line = BuildItemText(item);
    SendMessageW(list, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(line.c_str()));
      g_displayed_item_indices.push_back(index);
    ++shown;
    }
  }

  SendMessageW(list, WM_SETREDRAW, TRUE, 0);
  InvalidateRect(list, nullptr, TRUE);

  std::wstring status = std::wstring(UiText(TextId::kItemTable)) + L" ";
  status += std::to_wstring(g_items.size());
  status += UiText(TextId::kStatusRows);
  if (selected_category.empty()) {
    status += UiText(TextId::kStatusChooseCategory);
  } else {
    status += UiText(TextId::kStatusCurrentCategory);
    status += CategoryDisplay(selected_category);
    status += UiText(TextId::kStatusShown);
    status += std::to_wstring(shown);
    status += UiText(TextId::kStatusRows);
  }
  SetStatus(status);
}

void ApplyLanguageToUi() {
  if (!g_hwnd) return;
  SetWindowTextW(g_hwnd, UiText(TextId::kWindowTitle));
  SetText(IDC_LABEL_TITLE, UiText(TextId::kTitle));
  SetText(IDC_ENABLED, UiText(TextId::kEnabled));
  SetText(IDC_GROUND, UiText(TextId::kGround));
  SetText(IDC_CORPSE, UiText(TextId::kCorpse));
  SetText(IDC_FILTER, UiText(TextId::kFilter));
  SetText(IDC_FOREGROUND, UiText(TextId::kForeground));
  SetText(IDC_DEBUG, UiText(TextId::kDebug));
  SetText(IDC_LABEL_INTERACT_KEY, UiText(TextId::kInteractKey));
  SetText(IDC_LABEL_INTERVAL, UiText(TextId::kInterval));
  SetText(IDC_LABEL_TOGGLE_HOTKEY, UiText(TextId::kToggleHotkey));
  SetText(IDC_LABEL_CONFIG_HOTKEY, UiText(TextId::kConfigHotkey));
  SetText(IDC_LABEL_LANGUAGE, UiText(TextId::kLanguage));
  SetText(IDC_LABEL_CATEGORY, UiText(TextId::kCategory));
  SetText(IDC_LABEL_ITEMS, UiText(TextId::kItems));
  SetText(IDC_LABEL_SEARCH, UiText(TextId::kSearch));
  SetText(IDC_CLEAR_SEARCH, UiText(TextId::kClear));
  SetText(IDC_SAVE, UiText(TextId::kSave));
  SetText(IDC_RELOAD, UiText(TextId::kReload));
  SetText(IDC_OPEN_LOG, UiText(TextId::kLog));
  SetText(IDC_OPEN_ITEMS, UiText(TextId::kItemTable));
  FillLanguageCombo();
  RefreshCategoryList();
  RefreshItemList();
  InvalidateRect(g_hwnd, nullptr, TRUE);
}

void LoadConfigToUi() {
  EnsureIniDefaults();
  LoadLanguageSetting();
  LoadItems();

  SetCheck(IDC_ENABLED, ReadIniInt(L"General", L"Enabled", 1) != 0);
  SetCheck(IDC_DEBUG, ReadIniInt(L"General", L"DebugLog", 1) != 0);
  SetCheck(IDC_FOREGROUND, ReadIniInt(L"General", L"ForegroundOnly", 1) != 0);
  SetCheck(IDC_GROUND, ReadIniInt(L"Features", L"GroundLoot", 1) != 0);
  SetCheck(IDC_CORPSE, ReadIniInt(L"Features", L"CorpseLoot", 1) != 0);
  SetCheck(IDC_FILTER, ReadIniInt(L"ItemFilter", L"Enabled", 1) != 0);
  SetText(IDC_KEY_EDIT,
          NormalizeInteractKeyText(ReadIniString(L"General", L"InteractKey",
                                                 L"E"),
                                   L"E"));
  SetText(IDC_INTERVAL_EDIT,
          ReadIniString(L"General", L"TriggerIntervalMs", L"650"));
  SetText(IDC_TOGGLE_HOTKEY_EDIT,
          NormalizeHotkeyText(ReadIniString(L"General", L"ToggleHotkey",
                                            L"F9"),
                              L"F9"));
  SetText(IDC_CONFIG_HOTKEY_EDIT,
          NormalizeHotkeyText(ReadIniString(L"General", L"ConfigHotkey",
                                            L"F10"),
                              L"F10"));

  for (const auto& category : kCategories) {
    const size_t index = static_cast<size_t>(&category - kCategories);
    const int fallback = CategoryDefaultChecked(category.key) ? 1 : 0;
    g_category_enabled_ui[index] =
        ReadIniInt(L"ItemFilter", category.key, fallback) != 0;
  }

  RefreshCategoryList();
  RefreshItemList();
  ApplyLanguageToUi();
}

void SaveUiToConfig() {
  WriteIniInt(L"General", L"Enabled", GetCheck(IDC_ENABLED));
  WriteIniInt(L"General", L"DebugLog", GetCheck(IDC_DEBUG));
  WriteIniInt(L"General", L"ForegroundOnly", GetCheck(IDC_FOREGROUND));
  WriteIniInt(L"Features", L"GroundLoot", GetCheck(IDC_GROUND));
  WriteIniInt(L"Features", L"CorpseLoot", GetCheck(IDC_CORPSE));
  WriteIniInt(L"ItemFilter", L"Enabled", GetCheck(IDC_FILTER));

  SaveLanguageFromCombo();

  std::wstring interact_key = NormalizeInteractKeyText(GetText(IDC_KEY_EDIT), L"E");
  WriteIniString(L"General", L"InteractKey", interact_key);
  SetText(IDC_KEY_EDIT, interact_key);

  int interval = _wtoi(GetText(IDC_INTERVAL_EDIT).c_str());
  if (interval < 200) interval = 200;
  if (interval > 5000) interval = 5000;
  wchar_t interval_text[32]{};
  swprintf_s(interval_text, L"%d", interval);
  WriteIniString(L"General", L"TriggerIntervalMs", interval_text);
  SetText(IDC_INTERVAL_EDIT, interval_text);

  bool hotkey_conflict = false;
  std::wstring toggle_hotkey =
      SafeHotkeyText(GetText(IDC_TOGGLE_HOTKEY_EDIT), L"F9", &hotkey_conflict);
  std::wstring config_hotkey =
      SafeHotkeyText(GetText(IDC_CONFIG_HOTKEY_EDIT), L"F10", &hotkey_conflict);
  if (UpperAscii(toggle_hotkey) == UpperAscii(config_hotkey)) {
    hotkey_conflict = true;
    config_hotkey = L"F10";
    if (UpperAscii(toggle_hotkey) == UpperAscii(config_hotkey)) {
      config_hotkey = L"F11";
    }
  }
  WriteIniString(L"General", L"ToggleHotkey", toggle_hotkey);
  WriteIniString(L"General", L"ConfigHotkey", config_hotkey);
  SetText(IDC_TOGGLE_HOTKEY_EDIT, toggle_hotkey);
  SetText(IDC_CONFIG_HOTKEY_EDIT, config_hotkey);

  for (const auto& category : kCategories) {
    const size_t index = static_cast<size_t>(&category - kCategories);
    WriteIniInt(L"ItemFilter", category.key, g_category_enabled_ui[index]);
  }

  ApplyLanguageToUi();
  if (hotkey_conflict) {
    SetStatus(UiText(TextId::kStatusHotkeyConflict));
  } else {
    SetStatus(UiText(TextId::kStatusSaved));
  }
}

void SetSelectedItemBlocked(bool blocked) {
  const LRESULT sel = SendMessageW(Ctrl(IDC_ITEM_LIST), LB_GETCURSEL, 0, 0);
  if (sel == LB_ERR || sel < 0 ||
      static_cast<size_t>(sel) >= g_displayed_item_indices.size()) {
    SetStatus(UiText(TextId::kStatusSelectItem));
    return;
  }

  ItemRow& item = g_items[g_displayed_item_indices[static_cast<size_t>(sel)]];
  SetItemBlocked(item, blocked);

  const std::wstring name = CurrentLanguageItemName(item);
  RefreshItemList();
  std::wstring status = blocked ? UiText(TextId::kStatusBlocked)
                                : UiText(TextId::kStatusRestored);
  status += name;
  SetStatus(status);
}

void ToggleSelectedItemBlocked() {
  const LRESULT sel = SendMessageW(Ctrl(IDC_ITEM_LIST), LB_GETCURSEL, 0, 0);
  if (sel == LB_ERR || sel < 0 ||
      static_cast<size_t>(sel) >= g_displayed_item_indices.size()) {
    return;
  }
  const ItemRow& item = g_items[g_displayed_item_indices[static_cast<size_t>(sel)]];
  SetSelectedItemBlocked(!item.blocked);
}

void DrawCheckboxRow(DRAWITEMSTRUCT* draw, bool checked,
                     const std::wstring& text) {
  if (!draw || draw->itemID == static_cast<UINT>(-1)) return;

  const bool selected = (draw->itemState & ODS_SELECTED) != 0;
  HBRUSH bg = CreateSolidBrush(selected ? kColorSelect : kColorPanel);
  FillRect(draw->hDC, &draw->rcItem, bg);
  DeleteObject(bg);
  SetBkMode(draw->hDC, TRANSPARENT);
  SetTextColor(draw->hDC, selected ? RGB(255, 244, 214) : kColorText);

  RECT box = draw->rcItem;
  box.left += 4;
  box.top += 4;
  box.right = box.left + 16;
  box.bottom = box.top + 16;
  HBRUSH box_bg = CreateSolidBrush(kColorEdit);
  FillRect(draw->hDC, &box, box_bg);
  DeleteObject(box_bg);
  HBRUSH border = CreateSolidBrush(checked ? kColorAccent : RGB(92, 82, 62));
  FrameRect(draw->hDC, &box, border);
  DeleteObject(border);
  if (checked) {
    HPEN pen = CreatePen(PS_SOLID, 2, RGB(234, 196, 121));
    HPEN old_pen = reinterpret_cast<HPEN>(SelectObject(draw->hDC, pen));
    MoveToEx(draw->hDC, box.left + 3, box.top + 8, nullptr);
    LineTo(draw->hDC, box.left + 7, box.bottom - 4);
    LineTo(draw->hDC, box.right - 3, box.top + 3);
    SelectObject(draw->hDC, old_pen);
    DeleteObject(pen);
  }

  RECT text_rect = draw->rcItem;
  text_rect.left += 26;
  text_rect.right -= 4;
  DrawTextW(draw->hDC, text.c_str(), -1, &text_rect,
            DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);

  if (draw->itemState & ODS_FOCUS) {
    DrawFocusRect(draw->hDC, &draw->rcItem);
  }
}

bool IsCommandButton(UINT id) {
  return id == IDC_SAVE || id == IDC_RELOAD || id == IDC_OPEN_LOG ||
         id == IDC_OPEN_ITEMS || id == IDC_CLEAR_SEARCH;
}

void DrawCommandButton(DRAWITEMSTRUCT* draw) {
  if (!draw) return;
  const bool pressed = (draw->itemState & ODS_SELECTED) != 0;
  const bool disabled = (draw->itemState & ODS_DISABLED) != 0;
  HBRUSH bg = CreateSolidBrush(pressed ? kColorSelect : kColorPanelAlt);
  FillRect(draw->hDC, &draw->rcItem, bg);
  DeleteObject(bg);
  HBRUSH border = CreateSolidBrush(pressed ? RGB(228, 174, 90) : kColorAccent);
  FrameRect(draw->hDC, &draw->rcItem, border);
  DeleteObject(border);

  wchar_t text[128]{};
  GetWindowTextW(draw->hwndItem, text,
                 static_cast<int>(sizeof(text) / sizeof(text[0])));
  SetBkMode(draw->hDC, TRANSPARENT);
  SetTextColor(draw->hDC, disabled ? RGB(105, 98, 82) : kColorText);
  RECT text_rect = draw->rcItem;
  DrawTextW(draw->hDC, text, -1, &text_rect,
            DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_END_ELLIPSIS);

  if (draw->itemState & ODS_FOCUS) {
    RECT focus = draw->rcItem;
    InflateRect(&focus, -3, -3);
    DrawFocusRect(draw->hDC, &focus);
  }
}

void ToggleCategoryByIndex(size_t index) {
  if (index >= kCategoryCount) return;
  const bool new_enabled = !g_category_enabled_ui[index];
  g_category_enabled_ui[index] = new_enabled;
  WriteIniInt(L"ItemFilter", kCategories[index].key, new_enabled);
  SetAllItemsInCategoryBlocked(kCategories[index].key, !new_enabled);
  InvalidateRect(Ctrl(IDC_CATEGORY_LIST), nullptr, TRUE);
  RefreshItemList();

  std::wstring status = new_enabled ? UiText(TextId::kStatusCategoryEnabled)
                                    : UiText(TextId::kStatusCategoryDisabled);
  status += CategoryDisplay(kCategories[index].key);
  SetStatus(status);
}

void ToggleItemByDisplayedIndex(size_t displayed_index) {
  if (displayed_index >= g_displayed_item_indices.size()) return;
  ItemRow& item = g_items[g_displayed_item_indices[displayed_index]];
  const bool currently_checked = CategoryChecked(item.category) && !item.blocked;
  const bool next_checked = !currently_checked;
  if (next_checked) {
    const int category_index = CategoryIndexByKey(item.category);
    if (category_index >= 0 && !g_category_enabled_ui[static_cast<size_t>(category_index)]) {
      SetAllItemsInCategoryBlocked(item.category, true);
      g_category_enabled_ui[static_cast<size_t>(category_index)] = true;
      WriteIniInt(L"ItemFilter", kCategories[category_index].key, true);
      InvalidateRect(Ctrl(IDC_CATEGORY_LIST), nullptr, TRUE);
    }
    SetItemBlocked(item, false);
  } else {
    SetItemBlocked(item, true);
  }
  InvalidateRect(Ctrl(IDC_ITEM_LIST), nullptr, TRUE);

  std::wstring status = next_checked ? UiText(TextId::kStatusRestored)
                                     : UiText(TextId::kStatusBlocked);
  status += CurrentLanguageItemName(item);
  SetStatus(status);
}

int ListItemFromPoint(HWND hwnd, LPARAM lp) {
  const int y = static_cast<short>(HIWORD(lp));
  const int item_height = static_cast<int>(SendMessageW(hwnd, LB_GETITEMHEIGHT, 0, 0));
  if (item_height <= 0) return -1;
  const int top = static_cast<int>(SendMessageW(hwnd, LB_GETTOPINDEX, 0, 0));
  const int index = top + y / item_height;
  const int count = static_cast<int>(SendMessageW(hwnd, LB_GETCOUNT, 0, 0));
  if (index < 0 || index >= count) return -1;
  return index;
}

LRESULT CALLBACK CategoryListProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
  if (msg == WM_LBUTTONDOWN) {
    const int index = ListItemFromPoint(hwnd, lp);
    if (index >= 0) {
      const int x = static_cast<short>(LOWORD(lp));
      if (x < 26) {
        ToggleCategoryByIndex(static_cast<size_t>(index));
        return 0;
      }
      SendMessageW(hwnd, LB_SETCURSEL, index, 0);
      RefreshItemList();
      return 0;
    }
  }
  return CallWindowProcW(g_category_list_proc, hwnd, msg, wp, lp);
}

LRESULT CALLBACK ItemListProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
  if (msg == WM_LBUTTONDOWN) {
    const int index = ListItemFromPoint(hwnd, lp);
    if (index >= 0) {
      const int x = static_cast<short>(LOWORD(lp));
      if (x < 26) {
        ToggleItemByDisplayedIndex(static_cast<size_t>(index));
        return 0;
      }
      SendMessageW(hwnd, LB_SETCURSEL, index, 0);
      return 0;
    }
  }
  return CallWindowProcW(g_item_list_proc, hwnd, msg, wp, lp);
}

HWND AddControl(const wchar_t* cls, const wchar_t* text, DWORD style, int x,
                int y, int w, int h, int id) {
  HWND hwnd = CreateWindowExW(0, cls, text, style | WS_CHILD | WS_VISIBLE, x, y,
                              w, h, g_hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
                              g_instance, nullptr);
  if (hwnd && g_font) SendMessageW(hwnd, WM_SETFONT, (WPARAM)g_font, TRUE);
  return hwnd;
}

void AddLabel(const wchar_t* text, int x, int y, int w, int h, int id = 0) {
  AddControl(L"STATIC", text, WS_CHILD | WS_VISIBLE, x, y, w, h, id);
}

void CreateUi() {
  g_font = CreateFontW(-15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                       DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                       CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Microsoft YaHei UI");

  AddLabel(UiText(TextId::kTitle), 18, 12, 260, 22, IDC_LABEL_TITLE);
  AddLabel(UiText(TextId::kLanguage), 396, 12, 72, 22,
           IDC_LABEL_LANGUAGE);
  AddControl(L"COMBOBOX", L"",
             CBS_DROPDOWNLIST | WS_TABSTOP | WS_VSCROLL, 470, 8, 132, 120,
             IDC_LANGUAGE_COMBO);

  AddControl(L"BUTTON", UiText(TextId::kEnabled), BS_AUTOCHECKBOX | BS_FLAT, 18, 42, 80, 22,
             IDC_ENABLED);
  AddControl(L"BUTTON", UiText(TextId::kGround), BS_AUTOCHECKBOX | BS_FLAT, 112, 42, 80, 22,
             IDC_GROUND);
  AddControl(L"BUTTON", UiText(TextId::kCorpse), BS_AUTOCHECKBOX | BS_FLAT, 206, 42, 80, 22,
             IDC_CORPSE);
  AddControl(L"BUTTON", UiText(TextId::kFilter), BS_AUTOCHECKBOX | BS_FLAT, 300, 42, 80, 22,
             IDC_FILTER);

  AddControl(L"BUTTON", UiText(TextId::kForeground),
             BS_AUTOCHECKBOX | BS_FLAT, 18, 70, 190, 22, IDC_FOREGROUND);
  AddControl(L"BUTTON", UiText(TextId::kDebug), BS_AUTOCHECKBOX | BS_FLAT, 222, 70, 90,
             22, IDC_DEBUG);

  AddLabel(UiText(TextId::kInteractKey), 18, 104, 70, 22,
           IDC_LABEL_INTERACT_KEY);
  AddControl(L"EDIT", L"E", WS_BORDER | ES_AUTOHSCROLL, 88, 100, 58, 24,
             IDC_KEY_EDIT);
  AddLabel(UiText(TextId::kInterval), 158, 104, 82, 22,
           IDC_LABEL_INTERVAL);
  AddControl(L"EDIT", L"650", WS_BORDER | ES_NUMBER | ES_AUTOHSCROLL, 250, 100,
             70, 24, IDC_INTERVAL_EDIT);
  AddLabel(UiText(TextId::kToggleHotkey), 334, 104, 62, 22,
           IDC_LABEL_TOGGLE_HOTKEY);
  AddControl(L"EDIT", L"F9", WS_BORDER | ES_AUTOHSCROLL, 394, 100, 74, 24,
             IDC_TOGGLE_HOTKEY_EDIT);
  AddLabel(UiText(TextId::kConfigHotkey), 480, 104, 58, 22,
           IDC_LABEL_CONFIG_HOTKEY);
  AddControl(L"EDIT", L"F10", WS_BORDER | ES_AUTOHSCROLL, 538, 100, 64, 24,
             IDC_CONFIG_HOTKEY_EDIT);

  AddLabel(UiText(TextId::kCategory), 18, 142, 80, 22,
           IDC_LABEL_CATEGORY);
  HWND category_list = AddControl(
      L"LISTBOX", L"",
      WS_BORDER | WS_VSCROLL | LBS_NOTIFY | LBS_OWNERDRAWFIXED | LBS_HASSTRINGS,
      18, 166, 250, 350, IDC_CATEGORY_LIST);
  SendMessageW(category_list, LB_SETITEMHEIGHT, 0, 24);
  g_category_list_proc = reinterpret_cast<WNDPROC>(
      SetWindowLongPtrW(category_list, GWLP_WNDPROC,
                        reinterpret_cast<LONG_PTR>(CategoryListProc)));

  AddLabel(UiText(TextId::kItems), 290, 142, 70, 22, IDC_LABEL_ITEMS);
  AddLabel(UiText(TextId::kSearch), 290, 166, 42, 22, IDC_LABEL_SEARCH);
  AddControl(L"EDIT", L"", WS_BORDER | ES_AUTOHSCROLL, 334, 162, 160, 24,
             IDC_SEARCH_EDIT);
  AddControl(L"BUTTON", UiText(TextId::kClear), BS_OWNERDRAW, 504, 161, 64, 26,
             IDC_CLEAR_SEARCH);
  HWND item_list = AddControl(
      L"LISTBOX", L"",
      WS_BORDER | WS_VSCROLL | LBS_NOTIFY | LBS_OWNERDRAWFIXED | LBS_HASSTRINGS,
      290, 194, 278, 322, IDC_ITEM_LIST);
  SendMessageW(item_list, LB_SETITEMHEIGHT, 0, 24);
  g_item_list_proc = reinterpret_cast<WNDPROC>(
      SetWindowLongPtrW(item_list, GWLP_WNDPROC,
                        reinterpret_cast<LONG_PTR>(ItemListProc)));

  AddControl(L"BUTTON", UiText(TextId::kSave), BS_OWNERDRAW, 18, 536, 110, 32,
             IDC_SAVE);
  AddControl(L"BUTTON", UiText(TextId::kReload), BS_OWNERDRAW, 144, 536, 110, 32,
             IDC_RELOAD);
  AddControl(L"BUTTON", UiText(TextId::kLog), BS_OWNERDRAW, 270, 536, 110, 32,
             IDC_OPEN_LOG);
  AddControl(L"BUTTON", UiText(TextId::kItemTable), BS_OWNERDRAW, 396, 536, 110, 32,
             IDC_OPEN_ITEMS);

  AddControl(L"STATIC", L"", WS_BORDER | SS_LEFTNOWORDWRAP, 18, 584, 550, 36, IDC_STATUS);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
  switch (msg) {
    case WM_CREATE:
      g_hwnd = hwnd;
      CreateUi();
      LoadConfigToUi();
      return 0;
    case WM_DRAWITEM: {
      auto* draw = reinterpret_cast<DRAWITEMSTRUCT*>(lp);
      if (!draw) return 0;
      if (draw->CtlType == ODT_BUTTON && IsCommandButton(draw->CtlID)) {
        DrawCommandButton(draw);
        return TRUE;
      }
      if (draw->CtlID == IDC_CATEGORY_LIST &&
          draw->itemID < kCategoryCount) {
        DrawCheckboxRow(draw, g_category_enabled_ui[draw->itemID],
                        CategoryLabel(kCategories[draw->itemID]));
        return TRUE;
      }
      if (draw->CtlID == IDC_ITEM_LIST &&
          draw->itemID < g_displayed_item_indices.size()) {
        const ItemRow& item = g_items[g_displayed_item_indices[draw->itemID]];
        DrawCheckboxRow(draw, CategoryChecked(item.category) && !item.blocked,
                        BuildItemText(item));
        return TRUE;
      }
      return 0;
    }
    case WM_ERASEBKGND: {
      RECT rect{};
      GetClientRect(hwnd, &rect);
      FillRect(reinterpret_cast<HDC>(wp), &rect, g_bg_brush);
      return 1;
    }
    case WM_CTLCOLORSTATIC: {
      HDC hdc = reinterpret_cast<HDC>(wp);
      HWND child = reinterpret_cast<HWND>(lp);
      SetTextColor(hdc, GetDlgCtrlID(child) == IDC_STATUS ? kColorMuted : kColorText);
      SetBkMode(hdc, TRANSPARENT);
      return reinterpret_cast<LRESULT>(
          GetDlgCtrlID(child) == IDC_STATUS ? g_panel_brush : g_bg_brush);
    }
    case WM_CTLCOLOREDIT: {
      HDC hdc = reinterpret_cast<HDC>(wp);
      SetTextColor(hdc, kColorText);
      SetBkColor(hdc, kColorEdit);
      return reinterpret_cast<LRESULT>(g_edit_brush);
    }
    case WM_CTLCOLORLISTBOX: {
      HDC hdc = reinterpret_cast<HDC>(wp);
      SetTextColor(hdc, kColorText);
      SetBkColor(hdc, kColorPanel);
      return reinterpret_cast<LRESULT>(g_panel_brush);
    }
    case WM_CTLCOLORBTN: {
      HDC hdc = reinterpret_cast<HDC>(wp);
      SetTextColor(hdc, kColorText);
      SetBkMode(hdc, TRANSPARENT);
      return reinterpret_cast<LRESULT>(g_bg_brush);
    }
    case WM_COMMAND:
      if (LOWORD(wp) == IDC_CATEGORY_LIST && HIWORD(wp) == LBN_SELCHANGE) {
        RefreshItemList();
        return 0;
      }
      if (LOWORD(wp) == IDC_SEARCH_EDIT && HIWORD(wp) == EN_CHANGE) {
        RefreshItemList();
        return 0;
      }
      if (LOWORD(wp) == IDC_LANGUAGE_COMBO && HIWORD(wp) == CBN_SELCHANGE) {
        SaveLanguageFromCombo();
        ApplyLanguageToUi();
        SetStatus(UiText(TextId::kStatusSaved));
        return 0;
      }
      switch (LOWORD(wp)) {
        case IDC_SAVE:
          SaveUiToConfig();
          return 0;
        case IDC_RELOAD:
          LoadConfigToUi();
          return 0;
        case IDC_CLEAR_SEARCH:
          SetText(IDC_SEARCH_EDIT, L"");
          RefreshItemList();
          return 0;
        case IDC_OPEN_LOG:
          ShellExecuteW(hwnd, L"open", g_log_path.c_str(), nullptr, nullptr,
                        SW_SHOWNORMAL);
          return 0;
        case IDC_OPEN_ITEMS:
          ShellExecuteW(hwnd, L"open", g_items_path.c_str(), nullptr, nullptr,
                        SW_SHOWNORMAL);
          return 0;
        default:
          break;
      }
      break;
    case WM_DESTROY:
      if (g_font) {
        DeleteObject(g_font);
        g_font = nullptr;
      }
      if (g_bg_brush) {
        DeleteObject(g_bg_brush);
        g_bg_brush = nullptr;
      }
      if (g_panel_brush) {
        DeleteObject(g_panel_brush);
        g_panel_brush = nullptr;
      }
      if (g_edit_brush) {
        DeleteObject(g_edit_brush);
        g_edit_brush = nullptr;
      }
      PostQuitMessage(0);
      return 0;
    default:
      break;
  }
  return DefWindowProcW(hwnd, msg, wp, lp);
}

void BringConfigToFront(HWND hwnd) {
  if (!hwnd) return;
  ShowWindow(hwnd, IsIconic(hwnd) ? SW_RESTORE : SW_SHOWNORMAL);
  SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
               SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);

  HWND foreground = GetForegroundWindow();
  DWORD foreground_thread = foreground ? GetWindowThreadProcessId(foreground, nullptr) : 0;
  DWORD current_thread = GetCurrentThreadId();
  const BOOL attached = foreground_thread && foreground_thread != current_thread
                            ? AttachThreadInput(current_thread, foreground_thread, TRUE)
                            : FALSE;
  SetForegroundWindow(hwnd);
  SetActiveWindow(hwnd);
  SetFocus(hwnd);
  if (attached) AttachThreadInput(current_thread, foreground_thread, FALSE);
}

}  // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show_cmd) {
  g_single_instance =
      CreateMutexW(nullptr, TRUE, L"Local\\CrimsonAutolootCnConfigSingle");
  if (!g_single_instance || GetLastError() == ERROR_ALREADY_EXISTS) {
    HWND existing = FindWindowW(L"CrimsonAutolootConfigWindow", nullptr);
    if (existing) {
      BringConfigToFront(existing);
    }
    if (g_single_instance) CloseHandle(g_single_instance);
    return 0;
  }

  g_instance = instance;
  g_dir = ModuleDirectory();
  g_ini_path = g_dir + L"\\crimson_autoloot_cn.ini";
  g_default_ini_path = g_dir + L"\\crimson_autoloot_defaults.ini";
  g_log_path = g_dir + L"\\crimson_autoloot_cn.log";
  g_items_path = g_dir + L"\\crimson_autoloot_items.tsv";
  EnsureIniDefaults();
  LoadLanguageSetting();
  g_bg_brush = CreateSolidBrush(kColorBg);
  g_panel_brush = CreateSolidBrush(kColorPanel);
  g_edit_brush = CreateSolidBrush(kColorEdit);

  WNDCLASSW wc{};
  wc.lpfnWndProc = WndProc;
  wc.hInstance = instance;
  wc.lpszClassName = L"CrimsonAutolootConfigWindow";
  wc.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
  wc.hbrBackground = g_bg_brush;
  RegisterClassW(&wc);

  HWND hwnd = CreateWindowExW(
      WS_EX_TOPMOST, wc.lpszClassName, UiText(TextId::kWindowTitle),
      WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
      CW_USEDEFAULT, CW_USEDEFAULT, kWindowWidth, kWindowHeight, nullptr,
      nullptr, instance, nullptr);
  if (!hwnd) return 1;

  ShowWindow(hwnd, show_cmd);
  BringConfigToFront(hwnd);
  UpdateWindow(hwnd);

  MSG msg{};
  while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }
  if (g_single_instance) {
    CloseHandle(g_single_instance);
    g_single_instance = nullptr;
  }
  return 0;
}
