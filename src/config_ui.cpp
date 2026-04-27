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
  IDC_CAT_WEAPON,
  IDC_CAT_ARMOR,
  IDC_CAT_ACCESSORY,
  IDC_CAT_TOOL,
  IDC_CAT_HORSE_GEAR,
  IDC_CAT_PET_GEAR,
  IDC_CAT_VEHICLE_GEAR,
  IDC_CAT_EQUIPMENT,
  IDC_CAT_MISC,
};

struct CategoryControl {
  int id;
  const wchar_t* key;
  const wchar_t* label;
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
    {IDC_CAT_UNKNOWN, L"Unknown", L"\u672a\u77e5/\u672a\u89e3\u6790"},
    {IDC_CAT_CURRENCY, L"Currency", L"\u8d27\u5e01"},
    {IDC_CAT_MATERIAL, L"Material", L"\u6750\u6599"},
    {IDC_CAT_CONSUMABLE, L"Consumable", L"\u6d88\u8017\u54c1"},
    {IDC_CAT_FOOD, L"Food", L"\u98df\u7269/\u9c7c\u7c7b"},
    {IDC_CAT_RECIPE, L"Recipe", L"\u56fe\u7eb8/\u914d\u65b9"},
    {IDC_CAT_DOCUMENT, L"Document", L"\u4e66\u7c4d/\u6587\u4ef6"},
    {IDC_CAT_TRADE, L"Trade", L"\u8d38\u6613\u54c1"},
    {IDC_CAT_AMMO, L"Ammo", L"\u5f39\u836f"},
    {IDC_CAT_QUEST, L"Quest", L"\u4efb\u52a1\u7269\u54c1"},
    {IDC_CAT_WEAPON, L"Weapon", L"\u6b66\u5668"},
    {IDC_CAT_ARMOR, L"Armor", L"\u9632\u5177"},
    {IDC_CAT_ACCESSORY, L"Accessory", L"\u9970\u54c1"},
    {IDC_CAT_TOOL, L"Tool", L"\u5de5\u5177"},
    {IDC_CAT_HORSE_GEAR, L"HorseGear", L"\u9a6c\u5177"},
    {IDC_CAT_PET_GEAR, L"PetGear", L"\u5ba0\u7269\u88c5\u5907"},
    {IDC_CAT_VEHICLE_GEAR, L"VehicleGear", L"\u8f7d\u5177\u88c5\u5907"},
    {IDC_CAT_EQUIPMENT, L"Equipment", L"\u5176\u4ed6\u88c5\u5907"},
    {IDC_CAT_MISC, L"Misc", L"\u6742\u9879"},
};
constexpr size_t kCategoryCount = sizeof(kCategories) / sizeof(kCategories[0]);

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

std::wstring ModuleDirectory() {
  wchar_t path[MAX_PATH]{};
  GetModuleFileNameW(nullptr, path, MAX_PATH);
  std::wstring result(path);
  const size_t slash = result.find_last_of(L"\\/");
  if (slash != std::wstring::npos) result.resize(slash);
  return result;
}

int ReadIniInt(const wchar_t* section, const wchar_t* key, int fallback) {
  return GetPrivateProfileIntW(section, key, fallback, g_ini_path.c_str());
}

std::wstring ReadIniString(const wchar_t* section, const wchar_t* key,
                           const wchar_t* fallback) {
  wchar_t buffer[128]{};
  GetPrivateProfileStringW(section, key, fallback, buffer,
                           static_cast<DWORD>(sizeof(buffer) / sizeof(buffer[0])),
                           g_ini_path.c_str());
  return buffer;
}

void WriteIniInt(const wchar_t* section, const wchar_t* key, int value) {
  WritePrivateProfileStringW(section, key, value ? L"1" : L"0",
                             g_ini_path.c_str());
}

void WriteIniString(const wchar_t* section, const wchar_t* key,
                    const std::wstring& value) {
  WritePrivateProfileStringW(section, key, value.c_str(), g_ini_path.c_str());
}

HWND Ctrl(int id) { return GetDlgItem(g_hwnd, id); }

void SetCheck(int id, bool checked) {
  SendMessageW(Ctrl(id), BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED, 0);
}

bool GetCheck(int id) {
  return SendMessageW(Ctrl(id), BM_GETCHECK, 0, 0) == BST_CHECKED;
}

std::wstring GetText(int id) {
  wchar_t buffer[256]{};
  GetWindowTextW(Ctrl(id), buffer, static_cast<int>(sizeof(buffer) / sizeof(buffer[0])));
  return buffer;
}

void SetText(int id, const std::wstring& text) {
  SetWindowTextW(Ctrl(id), text.c_str());
}

void SetStatus(const std::wstring& text) { SetText(IDC_STATUS, text); }

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
  return category->label;
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

bool IsValidHotkeyKey(const std::wstring& token) {
  if (token.size() == 1) {
    const wchar_t ch = token[0];
    return (ch >= L'A' && ch <= L'Z') || (ch >= L'0' && ch <= L'9');
  }
  if (token.size() >= 2 && token[0] == L'F') {
    const int fn = _wtoi(token.c_str() + 1);
    return fn >= 1 && fn <= 12;
  }
  return token == L"INS" || token == L"INSERT" || token == L"DEL" ||
         token == L"DELETE" || token == L"HOME" || token == L"END" ||
         token == L"PGUP" || token == L"PAGEUP" || token == L"PGDN" ||
         token == L"PAGEDOWN";
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
      } else if (IsValidHotkeyKey(token)) {
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
  swprintf_s(buffer, L"%s  (%d)", category.label, count);
  return buffer;
}

std::wstring CurrentLanguageItemName(const ItemRow& item) {
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
  WritePrivateProfileStringW(L"General", L"StrictVersionCheck", L"0",
                             g_ini_path.c_str());
  WritePrivateProfileStringW(L"Features", L"GroundLoot", L"1",
                             g_ini_path.c_str());
  WritePrivateProfileStringW(L"Features", L"CorpseLoot", L"1",
                             g_ini_path.c_str());
  WritePrivateProfileStringW(L"ItemFilter", L"Enabled", L"1",
                             g_ini_path.c_str());
  for (const auto& category : kCategories) {
    WritePrivateProfileStringW(L"ItemFilter", category.key, L"1",
                               g_ini_path.c_str());
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

  std::wstring status = L"\u7269\u54c1\u8868 ";
  status += std::to_wstring(g_items.size());
  status += L" \u6761\u3002";
  if (selected_category.empty()) {
    status += L"\u8bf7\u5728\u5de6\u4fa7\u9009\u62e9\u4e00\u4e2a\u7c7b\u522b\u67e5\u770b\u7269\u54c1\u3002";
  } else {
    status += L"\u5f53\u524d\u7c7b\u522b ";
    status += CategoryDisplay(selected_category);
    status += L"\uff0c\u663e\u793a ";
    status += std::to_wstring(shown);
    status += L" \u6761\u3002";
  }
  SetStatus(status);
}

void LoadConfigToUi() {
  EnsureIniDefaults();
  LoadItems();

  SetCheck(IDC_ENABLED, ReadIniInt(L"General", L"Enabled", 1) != 0);
  SetCheck(IDC_DEBUG, ReadIniInt(L"General", L"DebugLog", 1) != 0);
  SetCheck(IDC_FOREGROUND, ReadIniInt(L"General", L"ForegroundOnly", 1) != 0);
  SetCheck(IDC_GROUND, ReadIniInt(L"Features", L"GroundLoot", 1) != 0);
  SetCheck(IDC_CORPSE, ReadIniInt(L"Features", L"CorpseLoot", 1) != 0);
  SetCheck(IDC_FILTER, ReadIniInt(L"ItemFilter", L"Enabled", 1) != 0);
  SetText(IDC_KEY_EDIT, ReadIniString(L"General", L"InteractKey", L"E"));
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
    g_category_enabled_ui[index] =
        ReadIniInt(L"ItemFilter", category.key, 1) != 0;
  }

  RefreshCategoryList();
  RefreshItemList();
}

void SaveUiToConfig() {
  WriteIniInt(L"General", L"Enabled", GetCheck(IDC_ENABLED));
  WriteIniInt(L"General", L"DebugLog", GetCheck(IDC_DEBUG));
  WriteIniInt(L"General", L"ForegroundOnly", GetCheck(IDC_FOREGROUND));
  WriteIniInt(L"Features", L"GroundLoot", GetCheck(IDC_GROUND));
  WriteIniInt(L"Features", L"CorpseLoot", GetCheck(IDC_CORPSE));
  WriteIniInt(L"ItemFilter", L"Enabled", GetCheck(IDC_FILTER));

  std::wstring key = GetText(IDC_KEY_EDIT);
  if (key.empty()) key = L"E";
  key.resize(1);
  if (key[0] >= L'a' && key[0] <= L'z') key[0] = key[0] - L'a' + L'A';
  WriteIniString(L"General", L"InteractKey", key);

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

  RefreshItemList();
  if (hotkey_conflict) {
    SetStatus(L"\u5df2\u4fdd\u5b58\u3002\u70ed\u952e\u4e0e\u6e38\u620f\u6309\u952e\u51b2\u7a81\uff0c\u5df2\u81ea\u52a8\u6539\u4e3a F9 / F10\u3002");
  } else {
    SetStatus(L"\u5df2\u4fdd\u5b58\u3002\u6e38\u620f\u5185\u63d2\u4ef6\u4f1a\u81ea\u52a8\u70ed\u52a0\u8f7d\uff0c\u65e0\u9700\u91cd\u542f\u3002");
  }
}

void SetSelectedItemBlocked(bool blocked) {
  const LRESULT sel = SendMessageW(Ctrl(IDC_ITEM_LIST), LB_GETCURSEL, 0, 0);
  if (sel == LB_ERR || sel < 0 ||
      static_cast<size_t>(sel) >= g_displayed_item_indices.size()) {
    SetStatus(L"\u8bf7\u5148\u5728\u7269\u54c1\u5217\u8868\u91cc\u9009\u4e2d\u4e00\u4e2a\u7269\u54c1\u3002");
    return;
  }

  ItemRow& item = g_items[g_displayed_item_indices[static_cast<size_t>(sel)]];
  SetItemBlocked(item, blocked);

  const std::wstring name = CurrentLanguageItemName(item);
  RefreshItemList();
  std::wstring status = blocked ? L"\u5df2\u8bbe\u4e3a\u4e0d\u62fe\u53d6\uff1a"
                                : L"\u5df2\u6062\u590d\u62fe\u53d6\uff1a";
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

  std::wstring status = new_enabled ? L"\u5df2\u5f00\u542f\u5206\u7c7b\u62fe\u53d6\u5e76\u6062\u590d\u8be5\u7c7b\u5168\u90e8\u7269\u54c1\uff1a"
                                    : L"\u5df2\u5173\u95ed\u5206\u7c7b\u62fe\u53d6\u5e76\u53d6\u6d88\u8be5\u7c7b\u5168\u90e8\u7269\u54c1\uff1a";
  status += kCategories[index].label;
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

  std::wstring status = next_checked ? L"\u5df2\u6062\u590d\u62fe\u53d6\uff1a"
                                    : L"\u5df2\u8bbe\u4e3a\u4e0d\u62fe\u53d6\uff1a";
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

void AddLabel(const wchar_t* text, int x, int y, int w, int h) {
  AddControl(L"STATIC", text, WS_CHILD | WS_VISIBLE, x, y, w, h, 0);
}

void CreateUi() {
  g_font = CreateFontW(-15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                       DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                       CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Microsoft YaHei UI");

  AddLabel(L"Crimson Desert \u81ea\u52a8\u62fe\u53d6", 18, 12, 260, 22);
  AddControl(L"BUTTON", L"\u542f\u7528", BS_AUTOCHECKBOX | BS_FLAT, 18, 42, 80, 22,
             IDC_ENABLED);
  AddControl(L"BUTTON", L"\u5730\u9762", BS_AUTOCHECKBOX | BS_FLAT, 112, 42, 80, 22,
             IDC_GROUND);
  AddControl(L"BUTTON", L"\u6478\u5c38", BS_AUTOCHECKBOX | BS_FLAT, 206, 42, 80, 22,
             IDC_CORPSE);
  AddControl(L"BUTTON", L"\u8fc7\u6ee4", BS_AUTOCHECKBOX | BS_FLAT, 300, 42, 80, 22,
             IDC_FILTER);

  AddControl(L"BUTTON", L"\u53ea\u5728\u6e38\u620f\u524d\u53f0\u65f6\u89e6\u53d1",
             BS_AUTOCHECKBOX | BS_FLAT, 18, 70, 190, 22, IDC_FOREGROUND);
  AddControl(L"BUTTON", L"\u8c03\u8bd5", BS_AUTOCHECKBOX | BS_FLAT, 222, 70, 90,
             22, IDC_DEBUG);

  AddLabel(L"\u4ea4\u4e92\u952e", 18, 104, 70, 22);
  AddControl(L"EDIT", L"E", WS_BORDER | ES_AUTOHSCROLL, 88, 100, 58, 24,
             IDC_KEY_EDIT);
  AddLabel(L"\u89e6\u53d1\u95f4\u9694", 158, 104, 82, 22);
  AddControl(L"EDIT", L"650", WS_BORDER | ES_NUMBER | ES_AUTOHSCROLL, 250, 100,
             70, 24, IDC_INTERVAL_EDIT);
  AddLabel(L"\u5f00\u5173\u952e", 334, 104, 62, 22);
  AddControl(L"EDIT", L"F9", WS_BORDER | ES_AUTOHSCROLL, 394, 100, 74, 24,
             IDC_TOGGLE_HOTKEY_EDIT);
  AddLabel(L"\u9762\u677f\u952e", 480, 104, 58, 22);
  AddControl(L"EDIT", L"F10", WS_BORDER | ES_AUTOHSCROLL, 538, 100, 64, 24,
             IDC_CONFIG_HOTKEY_EDIT);

  AddLabel(L"\u5206\u7c7b", 18, 142, 80, 22);
  HWND category_list = AddControl(
      L"LISTBOX", L"",
      WS_BORDER | WS_VSCROLL | LBS_NOTIFY | LBS_OWNERDRAWFIXED | LBS_HASSTRINGS,
      18, 166, 250, 350, IDC_CATEGORY_LIST);
  SendMessageW(category_list, LB_SETITEMHEIGHT, 0, 24);
  g_category_list_proc = reinterpret_cast<WNDPROC>(
      SetWindowLongPtrW(category_list, GWLP_WNDPROC,
                        reinterpret_cast<LONG_PTR>(CategoryListProc)));

  AddLabel(L"\u7269\u54c1", 290, 142, 70, 22);
  AddLabel(L"\u641c\u7d22", 290, 166, 42, 22);
  AddControl(L"EDIT", L"", WS_BORDER | ES_AUTOHSCROLL, 334, 162, 160, 24,
             IDC_SEARCH_EDIT);
  AddControl(L"BUTTON", L"\u6e05\u7a7a", BS_OWNERDRAW, 504, 161, 64, 26,
             IDC_CLEAR_SEARCH);
  HWND item_list = AddControl(
      L"LISTBOX", L"",
      WS_BORDER | WS_VSCROLL | LBS_NOTIFY | LBS_OWNERDRAWFIXED | LBS_HASSTRINGS,
      290, 194, 278, 322, IDC_ITEM_LIST);
  SendMessageW(item_list, LB_SETITEMHEIGHT, 0, 24);
  g_item_list_proc = reinterpret_cast<WNDPROC>(
      SetWindowLongPtrW(item_list, GWLP_WNDPROC,
                        reinterpret_cast<LONG_PTR>(ItemListProc)));

  AddControl(L"BUTTON", L"\u4fdd\u5b58", BS_OWNERDRAW, 18, 536, 110, 32,
             IDC_SAVE);
  AddControl(L"BUTTON", L"\u91cd\u8bfb", BS_OWNERDRAW, 144, 536, 110, 32,
             IDC_RELOAD);
  AddControl(L"BUTTON", L"\u65e5\u5fd7", BS_OWNERDRAW, 270, 536, 110, 32,
             IDC_OPEN_LOG);
  AddControl(L"BUTTON", L"\u7269\u54c1\u8868", BS_OWNERDRAW, 396, 536, 110, 32,
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
      WS_EX_TOPMOST, wc.lpszClassName, L"\u81ea\u52a8\u62fe\u53d6\u4e2d\u6587\u914d\u7f6e",
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
