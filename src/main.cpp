#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cwchar>
#include <cwctype>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

#ifndef _MSC_VER
#define __try if (true)
#define __except(x) else
#endif

namespace {

constexpr uint32_t kSupportedBuildTimestamp = 0x69EB11A5;
constexpr uintptr_t kPromptUpdateEntryRva = 0x00BD0C70;
constexpr uintptr_t kPromptTextAEntryRva = 0x00BD1377;
constexpr uintptr_t kPromptTextBEntryRva = 0x00BD13AF;
constexpr uintptr_t kPromptBranchRva = 0x00BD14E5;
constexpr uintptr_t kOriginalContinueRva = 0x00BD14FD;
constexpr uintptr_t kSkipPromptRva = 0x00BD15E0;
constexpr uintptr_t kPromptTextAReturnRva = 0x00BD139D;
constexpr uintptr_t kPromptTextBReturnRva = 0x00BD13C1;
constexpr uintptr_t kPromptTextALiteralRva = 0x04A517D0;
constexpr uintptr_t kPromptTextACallRva = 0x00AC4DC0;
constexpr uintptr_t kPromptTextBCallRva = 0x00AC4850;
constexpr size_t kPatchLen = 23;
constexpr size_t kPromptUpdatePatchLen = 15;
constexpr size_t kPromptTextAPatchLen = 0x26;
constexpr size_t kPromptTextBPatchLen = 0x12;

constexpr uint32_t kGroundLootType = 1;
constexpr uint32_t kGroundLootVariantType = 4;
constexpr uint32_t kGroundLootRelicType = 19;
constexpr uint32_t kCorpseLootType = 168;
constexpr WORD kDefaultInteractKey = 'E';
constexpr ULONGLONG kResolveCacheTtlMs = 1800;
constexpr ULONGLONG kPendingInputMaxAgeMs = 700;
enum HotkeyMod : uint8_t {
  kHotkeyAlt = 1 << 0,
  kHotkeyCtrl = 1 << 1,
  kHotkeyShift = 1 << 2,
};

enum ItemCategory : uint8_t {
  kCatUnknown = 0,
  kCatCurrency,
  kCatMaterial,
  kCatConsumable,
  kCatFood,
  kCatRecipe,
  kCatDocument,
  kCatTrade,
  kCatAmmo,
  kCatQuest,
  kCatWeapon,
  kCatArmor,
  kCatAccessory,
  kCatTool,
  kCatHorseGear,
  kCatPetGear,
  kCatVehicleGear,
  kCatEquipment,
  kCatMisc,
  kCatCount
};

struct ItemInfo {
  uint32_t key = 0;
  uint8_t category = kCatUnknown;
  std::wstring zh;
  std::wstring en;
  std::wstring internal;
};

struct ItemNameRef {
  std::wstring name;
  std::string utf8;
  uint32_t key = 0;
  uint8_t category = kCatUnknown;
};

struct ItemResolveResult {
  bool resolved = false;
  bool ambiguous = false;
  bool from_cache = false;
  bool text_match = false;
  uint32_t key = 0;
  uint8_t category = kCatUnknown;
  uint32_t offset = 0;
  uint8_t source = 0;
  uint32_t unique_keys = 0;
  uint32_t second_key = 0;
  int score = 0;
  int second_score = 0;
};

struct GroundResolveCacheEntry {
  uint32_t type = 0;
  uintptr_t target = 0;
  uintptr_t candidate = 0;
  ULONGLONG tick = 0;
  ItemResolveResult result{};
};

struct Hotkey {
  WORD vk = 0;
  uint8_t mods = 0;
};

struct ItemResolveStats {
  uint32_t unique_keys = 0;
  std::array<uint32_t, 16> keys{};
  std::array<int, 16> scores{};
};

struct GroundAllowConfirm {
  uint32_t type = 0;
  uintptr_t target = 0;
  uintptr_t candidate = 0;
  uint32_t key = 0;
  uint32_t offset = 0;
  uint8_t source = 0;
  bool ambiguous = false;
  uint32_t count = 0;
  ULONGLONG tick = 0;
};

struct PromptItemState {
  uint32_t key = 0;
  uint8_t category = kCatUnknown;
  uint8_t source = 0;
  uint32_t offset = 0;
  ULONGLONG tick = 0;
  uintptr_t a = 0;
  uintptr_t b = 0;
  uintptr_t c = 0;
  uintptr_t d = 0;
};

HMODULE g_self = nullptr;
uintptr_t g_game = 0;
std::wstring g_dir;
std::wstring g_support_dir;
std::wstring g_ini_path;
std::wstring g_default_ini_path;
std::wstring g_log_path;
std::wstring g_item_db_path;
std::wstring g_config_exe_path;
HANDLE g_single_instance = nullptr;
SRWLOCK g_blocked_items_lock = SRWLOCK_INIT;
SRWLOCK g_resolve_cache_lock = SRWLOCK_INIT;
SRWLOCK g_ground_confirm_lock = SRWLOCK_INIT;
SRWLOCK g_prompt_item_lock = SRWLOCK_INIT;

volatile LONG g_enabled = 1;
volatile LONG g_ground_enabled = 1;
volatile LONG g_corpse_enabled = 1;
volatile LONG g_debug_log = 1;
volatile LONG g_game_foreground_only = 1;
volatile LONG g_pending_ground = 0;
volatile LONG g_pending_corpse = 0;
volatile LONG64 g_pending_ground_tick = 0;
volatile LONG64 g_pending_corpse_tick = 0;
volatile LONG g_installed = 0;
volatile LONG g_prompt_hook_installed = 0;
volatile LONG g_prompt_text_hook_installed = 0;
volatile LONG g_version_ok = 0;
volatile LONG g_strict_version = 0;
volatile LONG g_trigger_interval_ms = 650;
volatile LONG g_interact_key = kDefaultInteractKey;
volatile LONG g_toggle_hotkey_vk = VK_F9;
volatile LONG g_toggle_hotkey_mods = 0;
volatile LONG g_config_hotkey_vk = VK_F10;
volatile LONG g_config_hotkey_mods = 0;
volatile LONG g_item_filter_enabled = 1;
std::array<volatile LONG, kCatCount> g_category_enabled{};
volatile LONG64 g_last_ground_target = 0;
volatile LONG64 g_last_ground_candidate = 0;
volatile LONG g_last_ground_item_key = 0;
volatile LONG g_last_ground_item_category = kCatUnknown;
volatile LONG g_last_ground_item_allowed = 1;
volatile LONG g_last_ground_item_blocked = 0;
volatile LONG g_last_ground_item_offset = 0;
volatile LONG g_last_ground_item_source = 0;
volatile LONG g_last_ground_item_ambiguous = 0;
volatile LONG g_last_ground_item_unique_keys = 0;
volatile LONG g_last_ground_item_confirmed = 0;
volatile LONG g_last_ground_item_text_match = 0;
volatile LONG g_last_interaction_type = 0;
volatile LONG g_last_prompt_item_key = 0;
volatile LONG g_last_prompt_queue_key = 0;
volatile LONG64 g_last_prompt_queue_tick = 0;
volatile LONG g_last_candidate_string_item_key = 0;
volatile LONG64 g_last_interaction_tick = 0;
volatile LONG64 g_last_corpse_target = 0;
volatile LONG64 g_last_corpse_candidate = 0;
volatile LONG64 g_last_interaction_context = 0;
std::array<volatile LONG64, 1024> g_seen{};
std::array<volatile LONG64, 1024> g_triggered{};
std::array<volatile LONG64, 1024> g_filtered{};
std::vector<ItemInfo> g_items;
std::vector<ItemNameRef> g_item_names;
std::unordered_map<std::wstring, ItemNameRef> g_item_name_lookup;
std::vector<uint32_t> g_blocked_items;
std::array<GroundResolveCacheEntry, 128> g_resolve_cache{};
volatile LONG g_resolve_cache_cursor = 0;
GroundAllowConfirm g_ground_confirm{};
PromptItemState g_prompt_item{};
HWND g_status_hwnd = nullptr;
HFONT g_status_font = nullptr;
std::wstring g_status_text;
ULONGLONG g_status_hide_at = 0;

void Log(const char* fmt, ...) {
  if (InterlockedCompareExchange(&g_debug_log, 0, 0) == 0 &&
      g_log_path.empty()) {
    return;
  }

  FILE* f = nullptr;
  _wfopen_s(&f, g_log_path.c_str(), L"a, ccs=UTF-8");
  if (!f) return;

  SYSTEMTIME st{};
  GetLocalTime(&st);
  fwprintf(f, L"[%04u-%02u-%02u %02u:%02u:%02u] ", st.wYear, st.wMonth,
           st.wDay, st.wHour, st.wMinute, st.wSecond);

  char buffer[1200]{};
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);
  fwprintf(f, L"%S\n", buffer);
  fclose(f);
}

std::wstring ModuleDirectory(HMODULE module) {
  wchar_t path[MAX_PATH]{};
  GetModuleFileNameW(module, path, MAX_PATH);
  std::wstring result(path);
  const size_t slash = result.find_last_of(L"\\/");
  if (slash != std::wstring::npos) result.resize(slash);
  return result;
}

BOOL CALLBACK FindGameWindowProc(HWND hwnd, LPARAM param) {
  DWORD pid = 0;
  GetWindowThreadProcessId(hwnd, &pid);
  if (pid != GetCurrentProcessId() || !IsWindowVisible(hwnd)) return TRUE;
  if (GetWindow(hwnd, GW_OWNER)) return TRUE;

  wchar_t cls[128]{};
  GetClassNameW(hwnd, cls, static_cast<int>(sizeof(cls) / sizeof(cls[0])));
  if (std::wcscmp(cls, L"CrimsonAutolootStatusOverlay") == 0) return TRUE;

  *reinterpret_cast<HWND*>(param) = hwnd;
  return FALSE;
}

HWND FindGameWindow() {
  HWND hwnd = nullptr;
  EnumWindows(FindGameWindowProc, reinterpret_cast<LPARAM>(&hwnd));
  return hwnd;
}

LRESULT CALLBACK StatusOverlayProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
  switch (msg) {
    case WM_PAINT: {
      PAINTSTRUCT ps{};
      HDC hdc = BeginPaint(hwnd, &ps);
      RECT rc{};
      GetClientRect(hwnd, &rc);

      HBRUSH bg = CreateSolidBrush(RGB(24, 22, 18));
      HPEN border = CreatePen(PS_SOLID, 1, RGB(191, 142, 73));
      HGDIOBJ old_brush = SelectObject(hdc, bg);
      HGDIOBJ old_pen = SelectObject(hdc, border);
      RoundRect(hdc, rc.left, rc.top, rc.right - 1, rc.bottom - 1, 10, 10);
      SelectObject(hdc, old_pen);
      SelectObject(hdc, old_brush);
      DeleteObject(border);
      DeleteObject(bg);

      SetBkMode(hdc, TRANSPARENT);
      SetTextColor(hdc, RGB(236, 220, 184));
      HFONT old_font = nullptr;
      if (g_status_font) {
        old_font = reinterpret_cast<HFONT>(SelectObject(hdc, g_status_font));
      }
      RECT text_rc = rc;
      InflateRect(&text_rc, -14, -6);
      DrawTextW(hdc, g_status_text.c_str(), -1, &text_rc,
                DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_END_ELLIPSIS);
      if (old_font) SelectObject(hdc, old_font);
      EndPaint(hwnd, &ps);
      return 0;
    }
    case WM_NCHITTEST:
      return HTTRANSPARENT;
    default:
      return DefWindowProcW(hwnd, msg, wp, lp);
  }
}

void EnsureStatusWindow() {
  if (g_status_hwnd && IsWindow(g_status_hwnd)) return;

  WNDCLASSW wc{};
  wc.lpfnWndProc = StatusOverlayProc;
  wc.hInstance = g_self;
  wc.lpszClassName = L"CrimsonAutolootStatusOverlay";
  wc.hbrBackground = nullptr;
  RegisterClassW(&wc);

  if (!g_status_font) {
    g_status_font = CreateFontW(-18, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                DEFAULT_PITCH, L"Microsoft YaHei UI");
  }

  g_status_hwnd = CreateWindowExW(
      WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW |
          WS_EX_NOACTIVATE,
      wc.lpszClassName, L"", WS_POPUP, 0, 0, 300, 48, nullptr, nullptr, g_self,
      nullptr);
  if (g_status_hwnd) {
    SetLayeredWindowAttributes(g_status_hwnd, 0, 232, LWA_ALPHA);
  }
}

void PositionStatusWindow() {
  if (!g_status_hwnd) return;
  RECT rc{};
  HWND game = FindGameWindow();
  if (game && GetWindowRect(game, &rc)) {
    const int width = 300;
    const int height = 48;
    const int x = rc.left + ((rc.right - rc.left) - width) / 2;
    const int y = rc.top + 90;
    SetWindowPos(g_status_hwnd, HWND_TOPMOST, x, y, width, height,
                 SWP_NOACTIVATE | SWP_SHOWWINDOW);
    return;
  }

  const int width = 300;
  const int height = 48;
  const int x = (GetSystemMetrics(SM_CXSCREEN) - width) / 2;
  SetWindowPos(g_status_hwnd, HWND_TOPMOST, x, 90, width, height,
               SWP_NOACTIVATE | SWP_SHOWWINDOW);
}

void ShowStatusToast(const wchar_t* text) {
  EnsureStatusWindow();
  if (!g_status_hwnd) return;
  g_status_text = text ? text : L"";
  g_status_hide_at = GetTickCount64() + 1800;
  PositionStatusWindow();
  InvalidateRect(g_status_hwnd, nullptr, TRUE);
}

void PumpStatusMessages() {
  MSG msg{};
  while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }
}

void UpdateStatusToast(ULONGLONG now) {
  if (!g_status_hwnd || !IsWindow(g_status_hwnd)) return;
  if (g_status_hide_at && now >= g_status_hide_at) {
    ShowWindow(g_status_hwnd, SW_HIDE);
    g_status_hide_at = 0;
  }
}

uint32_t ReadMainModuleTimestamp(HMODULE module) {
  auto* base = reinterpret_cast<const uint8_t*>(module);
  const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
  if (!dos || dos->e_magic != IMAGE_DOS_SIGNATURE) return 0;
  const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(
      base + static_cast<uint32_t>(dos->e_lfanew));
  if (!nt || nt->Signature != IMAGE_NT_SIGNATURE) return 0;
  return nt->FileHeader.TimeDateStamp;
}

bool ReadIniStringValue(const wchar_t* section, const wchar_t* key,
                        const wchar_t* fallback, wchar_t* out, DWORD out_len);

int ReadIniInt(const wchar_t* section, const wchar_t* key, int fallback) {
  wchar_t value[64]{};
  if (!ReadIniStringValue(section, key, L"", value,
                          static_cast<DWORD>(sizeof(value) / sizeof(value[0]))) ||
      !value[0]) {
    return fallback;
  }
  wchar_t* end = nullptr;
  const long parsed = std::wcstol(value, &end, 0);
  return end != value ? static_cast<int>(parsed) : fallback;
}

const char* CategoryName(uint8_t category) {
  switch (category) {
    case kCatCurrency:
      return "Currency";
    case kCatMaterial:
      return "Material";
    case kCatConsumable:
      return "Consumable";
    case kCatFood:
      return "Food";
    case kCatRecipe:
      return "Recipe";
    case kCatDocument:
      return "Document";
    case kCatTrade:
      return "Trade";
    case kCatAmmo:
      return "Ammo";
    case kCatQuest:
      return "Quest";
    case kCatWeapon:
      return "Weapon";
    case kCatArmor:
      return "Armor";
    case kCatAccessory:
      return "Accessory";
    case kCatTool:
      return "Tool";
    case kCatHorseGear:
      return "HorseGear";
    case kCatPetGear:
      return "PetGear";
    case kCatVehicleGear:
      return "VehicleGear";
    case kCatEquipment:
      return "Equipment";
    case kCatMisc:
      return "Misc";
    default:
      return "Unknown";
  }
}

const wchar_t* CategoryKey(uint8_t category) {
  switch (category) {
    case kCatCurrency:
      return L"Currency";
    case kCatMaterial:
      return L"Material";
    case kCatConsumable:
      return L"Consumable";
    case kCatFood:
      return L"Food";
    case kCatRecipe:
      return L"Recipe";
    case kCatDocument:
      return L"Document";
    case kCatTrade:
      return L"Trade";
    case kCatAmmo:
      return L"Ammo";
    case kCatQuest:
      return L"Quest";
    case kCatWeapon:
      return L"Weapon";
    case kCatArmor:
      return L"Armor";
    case kCatAccessory:
      return L"Accessory";
    case kCatTool:
      return L"Tool";
    case kCatHorseGear:
      return L"HorseGear";
    case kCatPetGear:
      return L"PetGear";
    case kCatVehicleGear:
      return L"VehicleGear";
    case kCatEquipment:
      return L"Equipment";
    case kCatMisc:
      return L"Misc";
    default:
      return L"Unknown";
  }
}

uint8_t ParseCategory(const char* value) {
  char lower[32]{};
  size_t n = 0;
  for (; value && value[n] && n + 1 < sizeof(lower); ++n) {
    lower[n] = static_cast<char>(std::tolower(static_cast<unsigned char>(value[n])));
  }
  lower[n] = 0;
  if (std::strcmp(lower, "currency") == 0) return kCatCurrency;
  if (std::strcmp(lower, "material") == 0) return kCatMaterial;
  if (std::strcmp(lower, "consumable") == 0) return kCatConsumable;
  if (std::strcmp(lower, "food") == 0) return kCatFood;
  if (std::strcmp(lower, "recipe") == 0) return kCatRecipe;
  if (std::strcmp(lower, "document") == 0) return kCatDocument;
  if (std::strcmp(lower, "trade") == 0) return kCatTrade;
  if (std::strcmp(lower, "ammo") == 0) return kCatAmmo;
  if (std::strcmp(lower, "quest") == 0) return kCatQuest;
  if (std::strcmp(lower, "weapon") == 0) return kCatWeapon;
  if (std::strcmp(lower, "armor") == 0) return kCatArmor;
  if (std::strcmp(lower, "accessory") == 0) return kCatAccessory;
  if (std::strcmp(lower, "tool") == 0) return kCatTool;
  if (std::strcmp(lower, "horsegear") == 0) return kCatHorseGear;
  if (std::strcmp(lower, "petgear") == 0) return kCatPetGear;
  if (std::strcmp(lower, "vehiclegear") == 0) return kCatVehicleGear;
  if (std::strcmp(lower, "equipment") == 0) return kCatEquipment;
  if (std::strcmp(lower, "misc") == 0) return kCatMisc;
  return kCatUnknown;
}

void TrimAsciiInPlace(char* text) {
  if (!text) return;
  char* begin = text;
  while (*begin == ' ' || *begin == '\t' || *begin == '\r' ||
         *begin == '\n') {
    ++begin;
  }
  if (begin != text) std::memmove(text, begin, std::strlen(begin) + 1);
  size_t len = std::strlen(text);
  while (len > 0 &&
         (text[len - 1] == ' ' || text[len - 1] == '\t' ||
          text[len - 1] == '\r' || text[len - 1] == '\n')) {
    text[--len] = 0;
  }
}

std::wstring Utf8ToWide(const char* text) {
  if (!text || !*text) return {};
  const int needed =
      MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text, -1, nullptr, 0);
  if (needed <= 1) return {};
  std::wstring out(static_cast<size_t>(needed - 1), L'\0');
  MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text, -1, &out[0], needed);
  return out;
}

std::string WideToUtf8(const std::wstring& text) {
  if (text.empty()) return {};
  const int needed =
      WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, nullptr, 0, nullptr,
                          nullptr);
  if (needed <= 1) return {};
  std::string out(static_cast<size_t>(needed - 1), '\0');
  WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, &out[0], needed, nullptr,
                      nullptr);
  return out;
}

std::wstring TrimIniWide(const std::wstring& text) {
  size_t begin = 0;
  size_t end = text.size();
  while (begin < end && iswspace(text[begin])) ++begin;
  while (end > begin && iswspace(text[end - 1])) --end;
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
  MultiByteToWideChar(codepage, 0, data, len, &out[0], needed);
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

bool ReadIniStringValue(const wchar_t* section, const wchar_t* key,
                        const wchar_t* fallback, wchar_t* out, DWORD out_len) {
  if (!out || out_len == 0) return false;
  out[0] = L'\0';
  GetPrivateProfileStringW(section, key, L"", out, out_len, g_ini_path.c_str());
  if (out[0]) return true;

  // Some UTF-8 BOM files make WinAPI treat the first section name as "\uFEFF[General]".
  std::wstring manual;
  if (ReadIniStringManual(section, key, &manual)) {
    wcsncpy_s(out, out_len, manual.c_str(), _TRUNCATE);
    return true;
  }
  if (fallback && fallback[0]) {
    wcsncpy_s(out, out_len, fallback, _TRUNCATE);
    return true;
  }
  return false;
}

void AddItemNameRef(const std::wstring& name, uint32_t key, uint8_t category) {
  if (name.size() < 2) return;
  bool has_cjk = false;
  bool ascii_only = true;
  for (wchar_t ch : name) {
    if (ch >= 0x4E00 && ch <= 0x9FFF) has_cjk = true;
    if (ch > 0x7F) ascii_only = false;
  }
  if (!has_cjk && ascii_only && name.size() < 6) return;
  g_item_names.push_back({name, WideToUtf8(name), key, category});
}

void EnsureDefaultIni() {
  if (GetFileAttributesW(g_ini_path.c_str()) != INVALID_FILE_ATTRIBUTES) {
    return;
  }

  if (GetFileAttributesW(g_default_ini_path.c_str()) != INVALID_FILE_ATTRIBUTES &&
      CopyFileW(g_default_ini_path.c_str(), g_ini_path.c_str(), FALSE)) {
    Log("default config restored: %S", g_default_ini_path.c_str());
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
  WritePrivateProfileStringW(L"ItemFilter", L"Unknown", L"1",
                             g_ini_path.c_str());
  WritePrivateProfileStringW(L"ItemFilter", L"Currency", L"1",
                             g_ini_path.c_str());
  WritePrivateProfileStringW(L"ItemFilter", L"Material", L"1",
                             g_ini_path.c_str());
  WritePrivateProfileStringW(L"ItemFilter", L"Consumable", L"1",
                             g_ini_path.c_str());
  WritePrivateProfileStringW(L"ItemFilter", L"Food", L"1",
                             g_ini_path.c_str());
  WritePrivateProfileStringW(L"ItemFilter", L"Recipe", L"1",
                             g_ini_path.c_str());
  WritePrivateProfileStringW(L"ItemFilter", L"Document", L"1",
                             g_ini_path.c_str());
  WritePrivateProfileStringW(L"ItemFilter", L"Trade", L"1",
                             g_ini_path.c_str());
  WritePrivateProfileStringW(L"ItemFilter", L"Ammo", L"1",
                             g_ini_path.c_str());
  WritePrivateProfileStringW(L"ItemFilter", L"Quest", L"1",
                             g_ini_path.c_str());
  WritePrivateProfileStringW(L"ItemFilter", L"Weapon", L"1",
                             g_ini_path.c_str());
  WritePrivateProfileStringW(L"ItemFilter", L"Armor", L"1",
                             g_ini_path.c_str());
  WritePrivateProfileStringW(L"ItemFilter", L"Accessory", L"1",
                             g_ini_path.c_str());
  WritePrivateProfileStringW(L"ItemFilter", L"Tool", L"1",
                             g_ini_path.c_str());
  WritePrivateProfileStringW(L"ItemFilter", L"HorseGear", L"1",
                             g_ini_path.c_str());
  WritePrivateProfileStringW(L"ItemFilter", L"PetGear", L"1",
                             g_ini_path.c_str());
  WritePrivateProfileStringW(L"ItemFilter", L"VehicleGear", L"1",
                             g_ini_path.c_str());
  WritePrivateProfileStringW(L"ItemFilter", L"Equipment", L"1",
                             g_ini_path.c_str());
  WritePrivateProfileStringW(L"ItemFilter", L"Misc", L"1",
                             g_ini_path.c_str());
}

void LoadBlockedItems() {
  std::vector<uint32_t> next;
  std::vector<wchar_t> section(128 * 1024);
  DWORD copied = GetPrivateProfileSectionW(
      L"BlockedItems", section.data(), static_cast<DWORD>(section.size()),
      g_ini_path.c_str());
  if (copied >= section.size() - 2) {
    section.resize(section.size() * 2);
    copied = GetPrivateProfileSectionW(
        L"BlockedItems", section.data(), static_cast<DWORD>(section.size()),
        g_ini_path.c_str());
  }

  wchar_t* entry = section.data();
  while (*entry) {
    wchar_t* eq = std::wcschr(entry, L'=');
    if (eq) {
      *eq = 0;
      const uint32_t key = static_cast<uint32_t>(std::wcstoul(entry, nullptr, 10));
      const int value = _wtoi(eq + 1);
      if (key != 0 && value != 0) next.push_back(key);
    }
    entry += std::wcslen(entry) + 1;
  }

  std::sort(next.begin(), next.end());
  next.erase(std::unique(next.begin(), next.end()), next.end());

  AcquireSRWLockExclusive(&g_blocked_items_lock);
  g_blocked_items.swap(next);
  ReleaseSRWLockExclusive(&g_blocked_items_lock);
}

size_t BlockedItemCount() {
  size_t count = 0;
  AcquireSRWLockShared(&g_blocked_items_lock);
  count = g_blocked_items.size();
  ReleaseSRWLockShared(&g_blocked_items_lock);
  return count;
}

WORD ReadInteractKey() {
  wchar_t value[32]{};
  ReadIniStringValue(L"General", L"InteractKey", L"E", value,
                     static_cast<DWORD>(sizeof(value) / sizeof(value[0])));
  for (wchar_t ch : value) {
    if (ch >= L'a' && ch <= L'z') return static_cast<WORD>(ch - L'a' + L'A');
    if ((ch >= L'A' && ch <= L'Z') || (ch >= L'0' && ch <= L'9')) {
      return static_cast<WORD>(ch);
    }
  }
  return kDefaultInteractKey;
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

WORD ParseHotkeyKey(const std::wstring& token) {
  if (token.size() == 1) {
    const wchar_t ch = token[0];
    if (ch >= L'A' && ch <= L'Z') return static_cast<WORD>(ch);
    if (ch >= L'0' && ch <= L'9') return static_cast<WORD>(ch);
  }
  if (token.size() >= 2 && token[0] == L'F') {
    const int fn = _wtoi(token.c_str() + 1);
    if (fn >= 1 && fn <= 12) return static_cast<WORD>(VK_F1 + fn - 1);
  }
  if (token == L"INSERT" || token == L"INS") return VK_INSERT;
  if (token == L"DELETE" || token == L"DEL") return VK_DELETE;
  if (token == L"HOME") return VK_HOME;
  if (token == L"END") return VK_END;
  if (token == L"PAGEUP" || token == L"PGUP") return VK_PRIOR;
  if (token == L"PAGEDOWN" || token == L"PGDN") return VK_NEXT;
  return 0;
}

Hotkey ParseHotkeyText(const wchar_t* text, Hotkey fallback) {
  if (!text || !*text) return fallback;
  std::wstring source = UpperAscii(text);
  std::replace(source.begin(), source.end(), L'-', L'+');

  Hotkey parsed{};
  size_t start = 0;
  while (start <= source.size()) {
    const size_t plus = source.find(L'+', start);
    std::wstring token = Trim(source.substr(
        start, plus == std::wstring::npos ? std::wstring::npos : plus - start));
    if (!token.empty()) {
      if (token == L"ALT" || token == L"MENU") {
        parsed.mods |= kHotkeyAlt;
      } else if (token == L"CTRL" || token == L"CONTROL") {
        parsed.mods |= kHotkeyCtrl;
      } else if (token == L"SHIFT") {
        parsed.mods |= kHotkeyShift;
      } else {
        const WORD vk = ParseHotkeyKey(token);
        if (vk != 0) parsed.vk = vk;
      }
    }
    if (plus == std::wstring::npos) break;
    start = plus + 1;
  }
  return parsed.vk == 0 ? fallback : parsed;
}

Hotkey ReadHotkey(const wchar_t* key, const wchar_t* fallback_text,
                  Hotkey fallback) {
  wchar_t value[64]{};
  ReadIniStringValue(L"General", key, fallback_text, value,
                     static_cast<DWORD>(sizeof(value) / sizeof(value[0])));
  return ParseHotkeyText(value, fallback);
}

void LoadConfig() {
  EnsureDefaultIni();

  const int enabled = ReadIniInt(L"General", L"Enabled", 1) ? 1 : 0;
  const int debug = ReadIniInt(L"General", L"DebugLog", 1) ? 1 : 0;
  const int foreground_only =
      ReadIniInt(L"General", L"ForegroundOnly", 1) ? 1 : 0;
  const int strict =
      ReadIniInt(L"General", L"StrictVersionCheck", 0) ? 1 : 0;
  int interval = ReadIniInt(L"General", L"TriggerIntervalMs", 650);
  if (interval < 200) interval = 200;
  if (interval > 5000) interval = 5000;

  const int ground = ReadIniInt(L"Features", L"GroundLoot", 1) ? 1 : 0;
  const int corpse = ReadIniInt(L"Features", L"CorpseLoot", 1) ? 1 : 0;
  const WORD interact_key = ReadInteractKey();
  const Hotkey toggle_hotkey =
      ReadHotkey(L"ToggleHotkey", L"F9", Hotkey{VK_F9, 0});
  const Hotkey config_hotkey =
      ReadHotkey(L"ConfigHotkey", L"F10", Hotkey{VK_F10, 0});
  const int filter_enabled =
      ReadIniInt(L"ItemFilter", L"Enabled", 1) ? 1 : 0;

  InterlockedExchange(&g_enabled, enabled);
  InterlockedExchange(&g_debug_log, debug);
  InterlockedExchange(&g_game_foreground_only, foreground_only);
  InterlockedExchange(&g_strict_version, strict);
  InterlockedExchange(&g_trigger_interval_ms, interval);
  InterlockedExchange(&g_interact_key, interact_key);
  InterlockedExchange(&g_toggle_hotkey_vk, toggle_hotkey.vk);
  InterlockedExchange(&g_toggle_hotkey_mods, toggle_hotkey.mods);
  InterlockedExchange(&g_config_hotkey_vk, config_hotkey.vk);
  InterlockedExchange(&g_config_hotkey_mods, config_hotkey.mods);
  InterlockedExchange(&g_ground_enabled, ground);
  InterlockedExchange(&g_corpse_enabled, corpse);
  InterlockedExchange(&g_item_filter_enabled, filter_enabled);
  for (uint8_t category = 0; category < kCatCount; ++category) {
    const int value = ReadIniInt(L"ItemFilter", CategoryKey(category), 1) ? 1 : 0;
    InterlockedExchange(&g_category_enabled[category], value);
  }
  LoadBlockedItems();
}

ULONGLONG ConfigWriteTime() {
  WIN32_FILE_ATTRIBUTE_DATA data{};
  if (!GetFileAttributesExW(g_ini_path.c_str(), GetFileExInfoStandard, &data)) {
    return 0;
  }
  return (static_cast<ULONGLONG>(data.ftLastWriteTime.dwHighDateTime) << 32) |
         data.ftLastWriteTime.dwLowDateTime;
}

bool IsGameForeground() {
  if (InterlockedCompareExchange(&g_game_foreground_only, 0, 0) == 0) {
    return true;
  }

  HWND hwnd = GetForegroundWindow();
  if (!hwnd) return false;
  DWORD pid = 0;
  GetWindowThreadProcessId(hwnd, &pid);
  return pid == GetCurrentProcessId();
}

bool IsReadableMemory(uintptr_t address, size_t bytes, bool heap_only = false);
bool IsItemCategoryAllowed(uint8_t category);
bool IsItemBlocked(uint32_t key);

bool IsReliableFilteredGroundItem(const ItemResolveResult& item) {
  return item.resolved && item.key != 0 && item.text_match;
}

bool IsGroundLootType(uint32_t type) {
  return type == kGroundLootType || type == kGroundLootVariantType ||
         type == kGroundLootRelicType;
}

void PressInteractKey() {
  const WORD interact_key =
      static_cast<WORD>(InterlockedCompareExchange(&g_interact_key, 0, 0));
  const WORD scan_code =
      static_cast<WORD>(MapVirtualKeyW(interact_key, MAPVK_VK_TO_VSC));
  if (scan_code == 0) return;

  INPUT inputs[2]{};
  inputs[0].type = INPUT_KEYBOARD;
  inputs[0].ki.wScan = scan_code;
  inputs[0].ki.dwFlags = KEYEVENTF_SCANCODE;
  SendInput(1, &inputs[0], sizeof(INPUT));

  Sleep(55);

  inputs[1].type = INPUT_KEYBOARD;
  inputs[1].ki.wScan = scan_code;
  inputs[1].ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
  SendInput(1, &inputs[1], sizeof(INPUT));
}

bool LookupItemCategory(uint32_t key, uint8_t* category) {
  const auto it = std::lower_bound(
      g_items.begin(), g_items.end(), key,
      [](const ItemInfo& item, uint32_t value) { return item.key < value; });
  if (it == g_items.end() || it->key != key) return false;
  if (category) *category = it->category;
  return true;
}

std::wstring TrimWideCopy(const std::wstring& text) {
  size_t begin = 0;
  size_t end = text.size();
  while (begin < end && iswspace(text[begin])) ++begin;
  while (end > begin && iswspace(text[end - 1])) --end;
  return text.substr(begin, end - begin);
}

void FillTextMatchResult(const ItemNameRef& ref, uint8_t source,
                         uint32_t offset, ItemResolveResult* result) {
  result->resolved = true;
  result->ambiguous = false;
  result->text_match = true;
  result->key = ref.key;
  result->category = ref.category;
  result->offset = offset;
  result->source = source;
  result->unique_keys = 1;
  result->score = 1000 + static_cast<int>(ref.name.size());
}

bool MatchItemNameText(const std::wstring& raw, uint8_t source,
                       uint32_t offset, ItemResolveResult* result) {
  if (!result || raw.size() < 2 || raw.size() > 160) return false;
  const std::wstring text = TrimWideCopy(raw);
  if (text.size() < 2) return false;

  const auto exact = g_item_name_lookup.find(text);
  if (exact != g_item_name_lookup.end()) {
    FillTextMatchResult(exact->second, source, offset, result);
    return true;
  }

  // Only compare extracted strings, not raw memory. This keeps the old
  // name-based matching behavior without scanning memory once per item name.
  if (text.size() < 4) return false;
  for (const ItemNameRef& ref : g_item_names) {
    if (ref.name.size() > text.size()) continue;
    if (ref.name.size() < 3) continue;
    if (text.find(ref.name) != std::wstring::npos) {
      FillTextMatchResult(ref, source, offset, result);
      return true;
    }
  }
  return false;
}

bool IsLikelyTextChar(wchar_t ch) {
  if (ch >= 0x4E00 && ch <= 0x9FFF) return true;
  if (ch >= 0x3400 && ch <= 0x4DBF) return true;
  if (ch >= 0x3000 && ch <= 0x303F) return true;
  if (ch >= 0xFF00 && ch <= 0xFFEF) return true;
  if (ch >= 0x20 && ch <= 0x7E) return true;
  return false;
}

bool ScanUtf16ItemNameInRegion(const uint8_t* data, size_t bytes,
                               uint8_t source, ItemResolveResult* result) {
  constexpr size_t kMaxChars = 96;
  for (size_t offset = 0; offset + sizeof(wchar_t) <= bytes; offset += 2) {
    std::wstring text;
    size_t pos = offset;
    while (pos + sizeof(wchar_t) <= bytes && text.size() < kMaxChars) {
      wchar_t ch = 0;
      std::memcpy(&ch, data + pos, sizeof(ch));
      if (!IsLikelyTextChar(ch)) break;
      text.push_back(ch);
      pos += sizeof(wchar_t);
    }
    if (text.size() >= 2 &&
        MatchItemNameText(text, source, static_cast<uint32_t>(offset),
                          result)) {
      return true;
    }
    if (pos > offset + sizeof(wchar_t)) offset = pos - sizeof(wchar_t);
  }
  return false;
}

bool ScanUtf8ItemNameInRegion(const uint8_t* data, size_t bytes,
                              uint8_t source, ItemResolveResult* result) {
  constexpr size_t kMaxBytes = 192;
  for (size_t offset = 0; offset < bytes; ++offset) {
    if (data[offset] == 0 || data[offset] < 0x20) continue;
    size_t end = offset;
    while (end < bytes && end - offset < kMaxBytes && data[end] != 0 &&
           (data[end] >= 0x20 || data[end] >= 0x80)) {
      ++end;
    }
    if (end - offset >= 4) {
      std::string raw(reinterpret_cast<const char*>(data + offset),
                      end - offset);
      std::wstring text = Utf8ToWide(raw.c_str());
      if (!text.empty() &&
          MatchItemNameText(text, source, static_cast<uint32_t>(offset),
                            result)) {
        return true;
      }
    }
    if (end > offset + 1) offset = end - 1;
  }
  return false;
}

bool ScanItemNameInRegion(uintptr_t base, size_t bytes, uint8_t source,
                          ItemResolveResult* result) {
  if (!result || !base || !IsReadableMemory(base, bytes, true)) return false;
  __try {
    const auto* data = reinterpret_cast<const uint8_t*>(base);
    if (ScanUtf16ItemNameInRegion(data, bytes, source, result)) return true;
    if (ScanUtf8ItemNameInRegion(data, bytes, source, result)) return true;
  } __except (EXCEPTION_EXECUTE_HANDLER) {
  }
  return false;
}

bool ResolveGroundItemByText(uintptr_t target, uintptr_t candidate,
                             uintptr_t context,
                             ItemResolveResult* result) {
  if (g_item_names.empty() || !result) return false;
  constexpr size_t kTextDirectScanBytes = 0x1000;
  constexpr size_t kTextPointerScanBytes = 0x500;
  constexpr size_t kTextPointerTableBytes = 0x240;
  const uintptr_t bases[] = {candidate, target, context};
  for (uintptr_t base : bases) {
    if (ScanItemNameInRegion(base, kTextDirectScanBytes, 4, result)) return true;
  }
  for (uintptr_t base : bases) {
    if (!base || !IsReadableMemory(base, kTextPointerTableBytes, true)) continue;
    __try {
      const auto* data = reinterpret_cast<const uint8_t*>(base);
      for (uint32_t offset = 0;
           offset + sizeof(uint64_t) <= kTextPointerTableBytes;
           offset += 8) {
        uint64_t ptr = 0;
        std::memcpy(&ptr, data + offset, sizeof(ptr));
        if (ScanItemNameInRegion(static_cast<uintptr_t>(ptr),
                                 kTextPointerScanBytes, 5, result)) {
          result->offset = offset;
          result->source = 5;
          return true;
        }
      }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
  }
  return false;
}

bool ResolvePromptItemByText(uintptr_t a, uintptr_t b, uintptr_t c,
                             uintptr_t d, ItemResolveResult* result) {
  if (g_item_names.empty() || !result) return false;
  constexpr size_t kPromptDirectScanBytes = 0x1000;
  constexpr size_t kPromptPointerScanBytes = 0x500;
  const uintptr_t bases[] = {b, a, c, d};
  for (uintptr_t base : bases) {
    if (ScanItemNameInRegion(base, kPromptDirectScanBytes, 6, result)) {
      return true;
    }
  }
  for (uintptr_t base : bases) {
    if (!base || !IsReadableMemory(base, 0x180, true)) continue;
    __try {
      const auto* data = reinterpret_cast<const uint8_t*>(base);
      for (uint32_t offset = 0; offset + sizeof(uint64_t) <= 0x180;
           offset += 8) {
        uint64_t ptr = 0;
        std::memcpy(&ptr, data + offset, sizeof(ptr));
        if (ScanItemNameInRegion(static_cast<uintptr_t>(ptr),
                                 kPromptPointerScanBytes, 7, result)) {
          result->offset = offset;
          result->source = 7;
          return true;
        }
      }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
  }
  return false;
}

void ClearPromptItem() {
  AcquireSRWLockExclusive(&g_prompt_item_lock);
  g_prompt_item = PromptItemState{};
  ReleaseSRWLockExclusive(&g_prompt_item_lock);
}

void QueueGroundFromPromptItem(const ItemResolveResult& item) {
  if (!item.resolved || item.key == 0) return;
  if (InterlockedCompareExchange(&g_enabled, 0, 0) == 0) return;
  if (InterlockedCompareExchange(&g_ground_enabled, 0, 0) == 0) return;

  const bool blocked = IsItemBlocked(item.key);
  const bool allowed = IsItemCategoryAllowed(item.category) && !blocked;
  const ULONGLONG now = GetTickCount64();
  const LONG key = static_cast<LONG>(item.key);
  const LONG previous_key = InterlockedCompareExchange(&g_last_prompt_queue_key, 0, 0);
  const ULONGLONG previous_tick = static_cast<ULONGLONG>(
      InterlockedCompareExchange64(&g_last_prompt_queue_tick, 0, 0));
  if (previous_key != key || now - previous_tick > 3000) {
    InterlockedExchange(&g_last_prompt_queue_key, key);
    InterlockedExchange64(&g_last_prompt_queue_tick, static_cast<LONG64>(now));
    Log("prompt text ground candidate: item=%lu category=%s allowed=%d blocked=%d source=%u offset=0x%X",
        item.key, CategoryName(item.category), allowed ? 1 : 0,
        blocked ? 1 : 0, item.source, item.offset);
  }
  if (!allowed) return;

  InterlockedExchange(&g_pending_ground, 1);
  InterlockedExchange64(&g_pending_ground_tick, static_cast<LONG64>(now));
}

void StorePromptItem(const ItemResolveResult& item, uintptr_t a, uintptr_t b,
                     uintptr_t c, uintptr_t d) {
  if (!item.resolved || item.ambiguous || item.key == 0) {
    ClearPromptItem();
    return;
  }
  PromptItemState next{};
  next.key = item.key;
  next.category = item.category;
  next.source = item.source;
  next.offset = item.offset;
  next.tick = GetTickCount64();
  next.a = a;
  next.b = b;
  next.c = c;
  next.d = d;

  AcquireSRWLockExclusive(&g_prompt_item_lock);
  g_prompt_item = next;
  ReleaseSRWLockExclusive(&g_prompt_item_lock);

  const LONG previous =
      InterlockedExchange(&g_last_prompt_item_key, static_cast<LONG>(item.key));
  if (previous != static_cast<LONG>(item.key)) {
    Log("prompt item matched: item=%lu category=%s source=%u offset=0x%X a=%p b=%p c=%p d=%p",
        item.key, CategoryName(item.category), item.source, item.offset,
        reinterpret_cast<void*>(a), reinterpret_cast<void*>(b),
        reinterpret_cast<void*>(c), reinterpret_cast<void*>(d));
  }
  QueueGroundFromPromptItem(item);
}

bool ResolveGroundItemFromPrompt(ItemResolveResult* result) {
  if (!result) return false;
  const ULONGLONG now = GetTickCount64();
  PromptItemState snapshot{};
  AcquireSRWLockShared(&g_prompt_item_lock);
  snapshot = g_prompt_item;
  ReleaseSRWLockShared(&g_prompt_item_lock);
  if (snapshot.key == 0 || now - snapshot.tick > 3000) return false;
  result->resolved = true;
  result->ambiguous = false;
  result->text_match = true;
  result->key = snapshot.key;
  result->category = snapshot.category;
  result->offset = snapshot.offset;
  result->source = snapshot.source;
  result->unique_keys = 1;
  result->score = 2000;
  return true;
}

extern "C" __declspec(noinline) void __fastcall
RecordPromptUpdate(uintptr_t a, uintptr_t b, uintptr_t c, uintptr_t d) {
  if (InterlockedCompareExchange(&g_enabled, 0, 0) == 0) return;

  ItemResolveResult item{};
  if (ResolvePromptItemByText(a, b, c, d, &item)) {
    StorePromptItem(item, a, b, c, d);
  } else {
    ClearPromptItem();
  }
}

bool IsReadableMemory(uintptr_t address, size_t bytes, bool heap_only) {
  if (address < 0x10000 || bytes == 0) return false;
  MEMORY_BASIC_INFORMATION mbi{};
  if (VirtualQuery(reinterpret_cast<const void*>(address), &mbi, sizeof(mbi)) !=
      sizeof(mbi)) {
    return false;
  }
  if (mbi.State != MEM_COMMIT) return false;
  if ((mbi.Protect & PAGE_GUARD) || (mbi.Protect & PAGE_NOACCESS)) return false;
  if (heap_only && mbi.Type == MEM_IMAGE) return false;
  const uintptr_t region_end =
      reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
  return address + bytes >= address && address + bytes <= region_end;
}

bool SafeRead32(uintptr_t address, uint32_t* value) {
  if (!value || !IsReadableMemory(address, sizeof(uint32_t))) return false;
  __try {
    std::memcpy(value, reinterpret_cast<const void*>(address), sizeof(*value));
    return true;
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    return false;
  }
}

bool SafeRead64(uintptr_t address, uint64_t* value) {
  if (!value || !IsReadableMemory(address, sizeof(uint64_t))) return false;
  __try {
    std::memcpy(value, reinterpret_cast<const void*>(address), sizeof(*value));
    return true;
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    return false;
  }
}

bool SafeReadBytes(uintptr_t address, void* buffer, size_t bytes,
                   bool heap_only = false) {
  if (!buffer || !IsReadableMemory(address, bytes, heap_only)) return false;
  __try {
    std::memcpy(buffer, reinterpret_cast<const void*>(address), bytes);
    return true;
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    return false;
  }
}

bool ReadMatchUtf8TextPointer(uintptr_t text_ptr, uint8_t source,
                              uint32_t offset,
                              ItemResolveResult* result) {
  if (!text_ptr || !result) return false;
  constexpr size_t kMaxBytes = 256;
  std::array<char, kMaxBytes + 1> buffer{};
  size_t bytes = kMaxBytes;
  while (bytes >= 8 && !IsReadableMemory(text_ptr, bytes)) bytes >>= 1;
  if (bytes < 8) return false;
  if (!SafeReadBytes(text_ptr, buffer.data(), bytes)) return false;

  size_t len = 0;
  while (len < bytes && buffer[len] != 0) {
    const unsigned char c = static_cast<unsigned char>(buffer[len]);
    if (c < 0x20 && c != '\t') return false;
    ++len;
  }
  if (len < 2 || len > 220) return false;
  buffer[len] = 0;

  const std::wstring text = Utf8ToWide(buffer.data());
  if (text.empty()) return false;
  return MatchItemNameText(text, source, offset, result);
}

bool ReadMatchUtf16TextPointer(uintptr_t text_ptr, uint8_t source,
                               uint32_t offset,
                               ItemResolveResult* result) {
  if (!text_ptr || !result) return false;
  constexpr size_t kMaxChars = 128;
  std::array<wchar_t, kMaxChars + 1> buffer{};
  size_t chars = kMaxChars;
  while (chars >= 4 && !IsReadableMemory(text_ptr, chars * sizeof(wchar_t))) {
    chars >>= 1;
  }
  if (chars < 4) return false;
  if (!SafeReadBytes(text_ptr, buffer.data(), chars * sizeof(wchar_t))) {
    return false;
  }

  size_t len = 0;
  while (len < chars && buffer[len] != 0) {
    if (!IsLikelyTextChar(buffer[len]) && !iswspace(buffer[len])) return false;
    ++len;
  }
  if (len < 2 || len > 120) return false;

  const std::wstring text(buffer.data(), len);
  return MatchItemNameText(text, source, offset, result);
}

bool MatchStringPointer(uintptr_t text_ptr, uint8_t source, uint32_t offset,
                        ItemResolveResult* result) {
  if (ReadMatchUtf8TextPointer(text_ptr, source, offset, result)) return true;
  if (ReadMatchUtf16TextPointer(text_ptr, source, offset, result)) return true;
  return false;
}

void StorePromptTextItem(uintptr_t text_ptr, uintptr_t entry, uintptr_t panel,
                         uintptr_t owner, uint8_t source) {
  ItemResolveResult item{};
  if (!MatchStringPointer(text_ptr, source, 0, &item)) return;
  StorePromptItem(item, text_ptr, entry, panel, owner);
}

extern "C" __declspec(noinline) void __fastcall
RecordPromptTextA(uintptr_t text_ptr, uintptr_t entry, uintptr_t panel,
                  uintptr_t owner) {
  if (InterlockedCompareExchange(&g_enabled, 0, 0) == 0) return;
  StorePromptTextItem(text_ptr, entry, panel, owner, 11);
}

extern "C" __declspec(noinline) void __fastcall
RecordPromptTextB(uintptr_t text_ptr, uintptr_t entry, uintptr_t panel,
                  uintptr_t owner) {
  if (InterlockedCompareExchange(&g_enabled, 0, 0) == 0) return;
  StorePromptTextItem(text_ptr, entry, panel, owner, 12);
}

void LogCandidateStringMatch(const ItemResolveResult& item, uint32_t field,
                             uintptr_t object, uintptr_t text_ptr) {
  const LONG previous = InterlockedExchange(
      &g_last_candidate_string_item_key, static_cast<LONG>(item.key));
  if (previous == static_cast<LONG>(item.key)) return;
  Log("candidate string matched: item=%lu category=%s source=%u offset=0x%X field=0x%X object=%p text=%p",
      item.key, CategoryName(item.category), item.source, item.offset, field,
      reinterpret_cast<void*>(object), reinterpret_cast<void*>(text_ptr));
}

bool TryCandidateStringField(uintptr_t candidate, uint32_t field,
                             ItemResolveResult* result) {
  uint64_t object64 = 0;
  if (!SafeRead64(candidate + field, &object64)) return false;
  const uintptr_t object = static_cast<uintptr_t>(object64);
  if (!object || !IsReadableMemory(object, sizeof(uint64_t))) return false;

  if (MatchStringPointer(object, 8, field, result)) {
    LogCandidateStringMatch(*result, field, object, object);
    return true;
  }

  constexpr uint32_t kSlots[] = {0x00, 0x08, 0x10, 0x18, 0x20,
                                 0x28, 0x30, 0x38, 0x40};
  for (uint32_t slot : kSlots) {
    uint64_t text64 = 0;
    if (!SafeRead64(object + slot, &text64)) continue;
    const uintptr_t text_ptr = static_cast<uintptr_t>(text64);
    const uint32_t packed_offset = field | (slot << 8);
    if (MatchStringPointer(text_ptr, 9, packed_offset, result)) {
      LogCandidateStringMatch(*result, field, object, text_ptr);
      return true;
    }

    if (!text_ptr || !IsReadableMemory(text_ptr, sizeof(uint64_t))) continue;
    constexpr uint32_t kChildSlots[] = {0x00, 0x08, 0x10, 0x18, 0x20};
    for (uint32_t child_slot : kChildSlots) {
      uint64_t nested64 = 0;
      if (!SafeRead64(text_ptr + child_slot, &nested64)) continue;
      const uintptr_t nested_ptr = static_cast<uintptr_t>(nested64);
      const uint32_t nested_offset =
          field | (slot << 8) | (child_slot << 16);
      if (MatchStringPointer(nested_ptr, 10, nested_offset, result)) {
        LogCandidateStringMatch(*result, field, object, nested_ptr);
        return true;
      }
    }
  }

  return false;
}

bool ResolveGroundItemFromCandidateStrings(uintptr_t candidate,
                                           ItemResolveResult* result) {
  if (g_item_names.empty() || !candidate || !result) return false;

  // The original prompt builder reads string objects from the interaction
  // entry around +0x30/+0x40. Prefer these exact fields over broad memory scans.
  constexpr uint32_t kStringFields[] = {0x30, 0x40, 0x28, 0x38, 0x48, 0x50};
  for (uint32_t field : kStringFields) {
    if (TryCandidateStringField(candidate, field, result)) return true;
  }
  return false;
}

int ItemCandidateScore(uint32_t key, uint8_t category, uint32_t offset,
                       uint8_t source) {
  int score = 10;
  if (key >= 1000) score += 30;
  if (key >= 100000) score += 10;
  if (category != kCatCurrency) score += 8;
  if (category == kCatQuest || category == kCatMaterial ||
      category == kCatWeapon || category == kCatArmor) {
    score += 4;
  }
  if (offset <= 0x200) score += 4;
  if (source == 1 || source == 2) score += 3;
  return score;
}

void TrackItemCandidate(ItemResolveStats* stats, uint32_t key, int score) {
  if (!stats || key == 0) return;
  const size_t stored =
      std::min<size_t>(stats->unique_keys, stats->keys.size());
  for (size_t i = 0; i < stored; ++i) {
    if (stats->keys[i] == key) {
      if (score > stats->scores[i]) stats->scores[i] = score;
      return;
    }
  }
  if (stats->unique_keys < stats->keys.size()) {
    stats->keys[stats->unique_keys] = key;
    stats->scores[stats->unique_keys] = score;
  }
  ++stats->unique_keys;
}

void ConsiderItemCandidate(ItemResolveResult* best, int* best_score,
                            ItemResolveStats* stats, uint32_t key,
                            uint32_t offset, uint8_t source) {
  uint8_t category = kCatUnknown;
  if (!LookupItemCategory(key, &category)) return;
  const int score = ItemCandidateScore(key, category, offset, source);
  TrackItemCandidate(stats, key, score);
  if (!best->resolved || score > *best_score) {
    best->resolved = true;
    best->key = key;
    best->category = category;
    best->offset = offset;
    best->source = source;
    best->score = score;
    *best_score = score;
  }
}

void ScanItemKeysInRegion(uintptr_t base, size_t bytes, uint8_t source,
                           ItemResolveResult* best, int* best_score,
                           ItemResolveStats* stats) {
  if (!IsReadableMemory(base, bytes, true)) return;
  __try {
    const auto* data = reinterpret_cast<const uint8_t*>(base);
    for (uint32_t offset = 0; offset + sizeof(uint32_t) <= bytes; offset += 4) {
      uint32_t value = 0;
      std::memcpy(&value, data + offset, sizeof(value));
      ConsiderItemCandidate(best, best_score, stats, value, offset, source);
    }
  } __except (EXCEPTION_EXECUTE_HANDLER) {
  }
}

ItemResolveResult ResolveGroundItem(uintptr_t target, uintptr_t candidate) {
  ItemResolveResult best{};
  int best_score = 0;
  ItemResolveStats stats{};

  constexpr size_t kDirectScanBytes = 0x420;
  constexpr size_t kPointerScanBytes = 0x220;
  ScanItemKeysInRegion(target, kDirectScanBytes, 1, &best, &best_score,
                       &stats);
  if (candidate && candidate != target) {
    ScanItemKeysInRegion(candidate, kDirectScanBytes, 2, &best, &best_score,
                         &stats);
  }

  const uintptr_t bases[] = {target, candidate};
  for (uintptr_t base : bases) {
    if (!base || !IsReadableMemory(base, 0x180, true)) continue;
    __try {
      const auto* data = reinterpret_cast<const uint8_t*>(base);
      for (uint32_t offset = 0; offset + sizeof(uint64_t) <= 0x180; offset += 8) {
        uint64_t ptr = 0;
        std::memcpy(&ptr, data + offset, sizeof(ptr));
        if (!IsReadableMemory(static_cast<uintptr_t>(ptr), kPointerScanBytes,
                              true)) {
          continue;
        }
        ScanItemKeysInRegion(static_cast<uintptr_t>(ptr), kPointerScanBytes, 3,
                             &best, &best_score, &stats);
      }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
  }

  if (best.resolved) {
    best.unique_keys = stats.unique_keys;
    const size_t stored =
        std::min<size_t>(stats.unique_keys, stats.keys.size());
    for (size_t i = 0; i < stored; ++i) {
      if (stats.keys[i] == best.key) continue;
      if (stats.scores[i] > best.second_score) {
        best.second_score = stats.scores[i];
        best.second_key = stats.keys[i];
      }
    }
    constexpr int kAmbiguousScoreMargin = 16;
    if (stats.unique_keys > stats.keys.size() ||
        (stats.unique_keys > 1 &&
         best.score - best.second_score < kAmbiguousScoreMargin)) {
      best.ambiguous = true;
    }
  }

  return best;
}

bool LookupGroundResolveCache(uint32_t type, uintptr_t target,
                              uintptr_t candidate, ItemResolveResult* result) {
  if (!result) return false;
  const ULONGLONG now = GetTickCount64();
  bool found = false;
  AcquireSRWLockShared(&g_resolve_cache_lock);
  for (const GroundResolveCacheEntry& entry : g_resolve_cache) {
    if (entry.type == type && entry.target == target &&
        entry.candidate == candidate && entry.tick != 0 &&
        now - entry.tick <= kResolveCacheTtlMs) {
      *result = entry.result;
      result->from_cache = true;
      found = true;
      break;
    }
  }
  ReleaseSRWLockShared(&g_resolve_cache_lock);
  return found;
}

void StoreGroundResolveCache(uint32_t type, uintptr_t target,
                             uintptr_t candidate,
                             const ItemResolveResult& result) {
  const LONG cursor = InterlockedIncrement(&g_resolve_cache_cursor);
  const size_t index =
      static_cast<size_t>(cursor) % static_cast<size_t>(g_resolve_cache.size());
  GroundResolveCacheEntry entry{};
  entry.type = type;
  entry.target = target;
  entry.candidate = candidate;
  entry.tick = GetTickCount64();
  entry.result = result;
  entry.result.from_cache = false;

  AcquireSRWLockExclusive(&g_resolve_cache_lock);
  g_resolve_cache[index] = entry;
  ReleaseSRWLockExclusive(&g_resolve_cache_lock);
}

bool ShouldTryGroundTextRefine(const ItemResolveResult& item) {
  if (!item.resolved || item.ambiguous) return true;
  if (item.text_match) return false;
  const uint8_t category =
      item.category < kCatCount ? item.category : kCatUnknown;
  if (item.key == 1 || item.key == 1000000 || category == kCatUnknown) {
    return true;
  }
  if (IsItemBlocked(item.key)) return true;
  if (!IsItemCategoryAllowed(category)) return true;
  return false;
}

bool TryGroundTextRefine(uintptr_t target, uintptr_t candidate,
                         uintptr_t context,
                         const ItemResolveResult& numeric,
                         ItemResolveResult* result) {
  if (!result) return false;
  ItemResolveResult text{};
  if (!ResolveGroundItemByText(target, candidate, context, &text)) return false;
  *result = text;
  if (!numeric.resolved || numeric.key != text.key) {
    Log("ground text refine: numeric_item=%lu numeric_category=%s numeric_source=%u numeric_offset=0x%X -> item=%lu category=%s source=%u offset=0x%X",
        numeric.key, CategoryName(numeric.category), numeric.source,
        numeric.offset, text.key, CategoryName(text.category), text.source,
        text.offset);
  }
  return true;
}

ItemResolveResult ResolveGroundItemCached(uint32_t type, uintptr_t target,
                                          uintptr_t candidate,
                                          uintptr_t context) {
  ItemResolveResult result{};
  if (LookupGroundResolveCache(type, target, candidate, &result)) {
    ItemResolveResult prompt{};
    if (ResolveGroundItemFromPrompt(&prompt)) {
      StoreGroundResolveCache(type, target, candidate, prompt);
      return prompt;
    }
    if (ShouldTryGroundTextRefine(result)) {
      ItemResolveResult text{};
      if (TryGroundTextRefine(target, candidate, context, result, &text)) {
        StoreGroundResolveCache(type, target, candidate, text);
        return text;
      }
    }
    return result;
  }
  if (ResolveGroundItemFromPrompt(&result)) {
    StoreGroundResolveCache(type, target, candidate, result);
    return result;
  }
  result = ResolveGroundItem(target, candidate);
  if (ShouldTryGroundTextRefine(result)) {
    ItemResolveResult text{};
    if (TryGroundTextRefine(target, candidate, context, result, &text)) {
      StoreGroundResolveCache(type, target, candidate, text);
      return text;
    }
  }
  StoreGroundResolveCache(type, target, candidate, result);
  return result;
}

bool IsItemCategoryAllowed(uint8_t category) {
  if (InterlockedCompareExchange(&g_item_filter_enabled, 0, 0) == 0) {
    return true;
  }
  if (category >= kCatCount) category = kCatUnknown;
  return InterlockedCompareExchange(&g_category_enabled[category], 0, 0) != 0;
}

bool IsItemBlocked(uint32_t key) {
  if (key == 0) return false;
  bool blocked = false;
  AcquireSRWLockShared(&g_blocked_items_lock);
  blocked = std::binary_search(g_blocked_items.begin(), g_blocked_items.end(), key);
  ReleaseSRWLockShared(&g_blocked_items_lock);
  return blocked;
}

void ResetGroundAllowConfirm() {
  AcquireSRWLockExclusive(&g_ground_confirm_lock);
  g_ground_confirm = GroundAllowConfirm{};
  ReleaseSRWLockExclusive(&g_ground_confirm_lock);
}

void ResetLootRuntimeState(const char* reason) {
  InterlockedExchange(&g_pending_ground, 0);
  InterlockedExchange(&g_pending_corpse, 0);
  InterlockedExchange64(&g_pending_ground_tick, 0);
  InterlockedExchange64(&g_pending_corpse_tick, 0);
  InterlockedExchange(&g_last_prompt_item_key, 0);
  InterlockedExchange(&g_last_prompt_queue_key, 0);
  InterlockedExchange64(&g_last_prompt_queue_tick, 0);
  ClearPromptItem();
  ResetGroundAllowConfirm();
  AcquireSRWLockExclusive(&g_resolve_cache_lock);
  g_resolve_cache = {};
  ReleaseSRWLockExclusive(&g_resolve_cache_lock);
  InterlockedExchange(&g_resolve_cache_cursor, 0);
  Log("runtime loot state reset: %s", reason ? reason : "unknown");
}

bool ConfirmAllowedGroundItem(uint32_t type, uintptr_t target,
                              uintptr_t candidate,
                              const ItemResolveResult& item) {
  if (!item.resolved || item.key == 0) return false;

  const ULONGLONG now = GetTickCount64();
  // After sheathing the weapon the game may only emit one interaction callback
  // for the prompt, so ambiguous-but-allowed items cannot rely on repeated
  // callbacks for confirmation.
  const uint32_t required_count = item.ambiguous ? 1u : 2u;
  bool confirmed = false;
  AcquireSRWLockExclusive(&g_ground_confirm_lock);
  const bool same = g_ground_confirm.type == type &&
                    g_ground_confirm.target == target &&
                    g_ground_confirm.candidate == candidate &&
                    g_ground_confirm.key == item.key &&
                    g_ground_confirm.offset == item.offset &&
                    g_ground_confirm.source == item.source &&
                    g_ground_confirm.ambiguous == item.ambiguous &&
                    now - g_ground_confirm.tick <= 5000;
  if (!same) {
    g_ground_confirm.type = type;
    g_ground_confirm.target = target;
    g_ground_confirm.candidate = candidate;
    g_ground_confirm.key = item.key;
    g_ground_confirm.offset = item.offset;
    g_ground_confirm.source = item.source;
    g_ground_confirm.ambiguous = item.ambiguous;
    g_ground_confirm.count = 1;
    g_ground_confirm.tick = now;
  } else {
    if (g_ground_confirm.count < required_count) {
      ++g_ground_confirm.count;
    }
    g_ground_confirm.tick = now;
  }
  confirmed = g_ground_confirm.count >= required_count;
  ReleaseSRWLockExclusive(&g_ground_confirm_lock);
  return confirmed;
}

extern "C" __declspec(noinline) void __fastcall
RecordInteraction(uint32_t type, uintptr_t target, uintptr_t candidate,
                  uintptr_t context) {
  InterlockedExchange(&g_last_interaction_type, static_cast<LONG>(type));
  InterlockedExchange64(&g_last_interaction_context,
                        static_cast<LONG64>(context));
  if (IsGroundLootType(type)) {
    InterlockedExchange64(&g_last_ground_target, static_cast<LONG64>(target));
    InterlockedExchange64(&g_last_ground_candidate,
                          static_cast<LONG64>(candidate));
  } else if (type == kCorpseLootType) {
    InterlockedExchange64(&g_last_corpse_target, static_cast<LONG64>(target));
    InterlockedExchange64(&g_last_corpse_candidate,
                          static_cast<LONG64>(candidate));
  }

  if (InterlockedCompareExchange(&g_enabled, 0, 0) == 0) return;

  if (type < g_seen.size()) {
    InterlockedIncrement64(&g_seen[type]);
  }

  if (InterlockedCompareExchange(&g_version_ok, 0, 0) == 0 &&
      InterlockedCompareExchange(&g_strict_version, 0, 0) != 0) {
    return;
  }
  const ULONGLONG now = GetTickCount64();
  const ULONGLONG last_interaction = static_cast<ULONGLONG>(
      InterlockedExchange64(&g_last_interaction_tick, static_cast<LONG64>(now)));
  if (last_interaction != 0 && now - last_interaction > 8000) {
    ResetLootRuntimeState("interaction idle or save reload");
  }

  if (IsGroundLootType(type) &&
      InterlockedCompareExchange(&g_ground_enabled, 0, 0) != 0) {
    ItemResolveResult item{};
    const bool filter_active =
        InterlockedCompareExchange(&g_item_filter_enabled, 0, 0) != 0;
    if (filter_active) {
      item = ResolveGroundItemCached(type, target, candidate, context);
    }
    const bool known_item =
        filter_active ? IsReliableFilteredGroundItem(item) : item.resolved;
    const uint8_t category = known_item ? item.category : kCatUnknown;
    const bool blocked = known_item && IsItemBlocked(item.key);
    bool allowed = IsItemCategoryAllowed(category) && !blocked;
    bool confirmed = true;
    if (filter_active && item.resolved && !item.text_match) allowed = false;
    InterlockedExchange64(&g_last_ground_target, static_cast<LONG64>(target));
    InterlockedExchange64(&g_last_ground_candidate,
                          static_cast<LONG64>(candidate));
    InterlockedExchange(&g_last_ground_item_key,
                        item.resolved ? static_cast<LONG>(item.key) : 0);
    InterlockedExchange(&g_last_ground_item_category, category);
    InterlockedExchange(&g_last_ground_item_allowed, allowed ? 1 : 0);
    InterlockedExchange(&g_last_ground_item_blocked, blocked ? 1 : 0);
    InterlockedExchange(&g_last_ground_item_offset,
                        item.resolved ? static_cast<LONG>(item.offset) : -1);
    InterlockedExchange(&g_last_ground_item_source,
                        item.resolved ? static_cast<LONG>(item.source) : 0);
    InterlockedExchange(&g_last_ground_item_ambiguous,
                        item.ambiguous ? 1 : 0);
    InterlockedExchange(&g_last_ground_item_unique_keys,
                        static_cast<LONG>(item.unique_keys));
    InterlockedExchange(&g_last_ground_item_confirmed,
                        confirmed ? 1 : 0);
    InterlockedExchange(&g_last_ground_item_text_match,
                        item.text_match ? 1 : 0);
    if (!allowed || !confirmed) {
      InterlockedExchange(&g_pending_ground, 0);
      InterlockedExchange64(&g_pending_ground_tick, 0);
      if (!allowed) ResetGroundAllowConfirm();
      if (type < g_filtered.size()) InterlockedIncrement64(&g_filtered[type]);
      return;
    }
    InterlockedExchange(&g_pending_ground, 1);
    InterlockedExchange64(&g_pending_ground_tick,
                          static_cast<LONG64>(GetTickCount64()));
    if (type < g_triggered.size()) InterlockedIncrement64(&g_triggered[type]);
    return;
  }

  if (type == kCorpseLootType &&
      InterlockedCompareExchange(&g_corpse_enabled, 0, 0) != 0) {
    InterlockedExchange64(&g_last_corpse_target, static_cast<LONG64>(target));
    InterlockedExchange64(&g_last_corpse_candidate,
                          static_cast<LONG64>(candidate));
    InterlockedExchange(&g_pending_corpse, 1);
    InterlockedExchange64(&g_pending_corpse_tick,
                          static_cast<LONG64>(GetTickCount64()));
    if (type < g_triggered.size()) InterlockedIncrement64(&g_triggered[type]);
    return;
  }
}

void EmitBytes(std::vector<uint8_t>& code, std::initializer_list<uint8_t> data) {
  code.insert(code.end(), data.begin(), data.end());
}

void Emit32(std::vector<uint8_t>& code, uint32_t value) {
  for (int i = 0; i < 4; ++i) code.push_back((value >> (i * 8)) & 0xff);
}

void Emit64(std::vector<uint8_t>& code, uint64_t value) {
  for (int i = 0; i < 8; ++i) code.push_back((value >> (i * 8)) & 0xff);
}

void EmitMovRaxImm64(std::vector<uint8_t>& code, uint64_t value) {
  EmitBytes(code, {0x48, 0xB8});
  Emit64(code, value);
}

void EmitMovR9Imm64(std::vector<uint8_t>& code, uint64_t value) {
  EmitBytes(code, {0x49, 0xB9});
  Emit64(code, value);
}

void EmitAbsJump(std::vector<uint8_t>& code, uint64_t target) {
  EmitBytes(code, {0xFF, 0x25, 0x00, 0x00, 0x00, 0x00});
  Emit64(code, target);
}

size_t EmitJccRel32Placeholder(std::vector<uint8_t>& code, uint8_t cc) {
  EmitBytes(code, {0x0F, cc});
  const size_t disp = code.size();
  Emit32(code, 0);
  return disp;
}

void PatchRel32(std::vector<uint8_t>& code, size_t disp_offset,
                size_t target_offset) {
  const int64_t from = static_cast<int64_t>(disp_offset + 4);
  const int64_t to = static_cast<int64_t>(target_offset);
  const int32_t rel = static_cast<int32_t>(to - from);
  std::memcpy(code.data() + disp_offset, &rel, sizeof(rel));
}

void EmitSaveVolatile(std::vector<uint8_t>& code) {
  EmitBytes(code, {0x50, 0x51, 0x52, 0x41, 0x50, 0x41, 0x51,
                   0x41, 0x52, 0x41, 0x53, 0x48, 0x81, 0xEC, 0x88,
                   0x00, 0x00, 0x00});
  EmitBytes(code, {0x0F, 0x11, 0x44, 0x24, 0x20});
  EmitBytes(code, {0x0F, 0x11, 0x4C, 0x24, 0x30});
  EmitBytes(code, {0x0F, 0x11, 0x54, 0x24, 0x40});
  EmitBytes(code, {0x0F, 0x11, 0x5C, 0x24, 0x50});
  EmitBytes(code, {0x0F, 0x11, 0x64, 0x24, 0x60});
  EmitBytes(code, {0x0F, 0x11, 0x6C, 0x24, 0x70});
}

void EmitRestoreVolatile(std::vector<uint8_t>& code) {
  EmitBytes(code, {0x0F, 0x10, 0x44, 0x24, 0x20});
  EmitBytes(code, {0x0F, 0x10, 0x4C, 0x24, 0x30});
  EmitBytes(code, {0x0F, 0x10, 0x54, 0x24, 0x40});
  EmitBytes(code, {0x0F, 0x10, 0x5C, 0x24, 0x50});
  EmitBytes(code, {0x0F, 0x10, 0x64, 0x24, 0x60});
  EmitBytes(code, {0x0F, 0x10, 0x6C, 0x24, 0x70});
  EmitBytes(code, {0x48, 0x81, 0xC4, 0x88, 0x00, 0x00, 0x00,
                   0x41, 0x5B, 0x41, 0x5A, 0x41, 0x59, 0x41, 0x58,
                   0x5A, 0x59, 0x58});
}

std::vector<uint8_t> BuildPromptBranchStub() {
  std::vector<uint8_t> code;
  code.reserve(256);

  EmitSaveVolatile(code);
  EmitBytes(code, {0x41, 0x0F, 0xB7, 0x4D, 0x10});  // movzx ecx,[r13+10h]
  EmitBytes(code, {0x48, 0x89, 0xFA, 0x4D, 0x89, 0xE8,
                   0x4D, 0x89, 0xF1});  // rdx=rdi r8=r13 r9=r14
  EmitMovRaxImm64(code, reinterpret_cast<uint64_t>(&RecordInteraction));
  EmitBytes(code, {0xFF, 0xD0});
  EmitRestoreVolatile(code);

  // Reproduce the exact overwritten game branch. Do not force type 168 into
  // the old autoloot continuation; that route caused the repeated crashes.
  EmitBytes(code, {0x40, 0x80, 0xFE, 0x02});  // cmp sil,2
  const size_t jne_not_sil2 = EmitJccRel32Placeholder(code, 0x85);
  EmitAbsJump(code, g_game + kSkipPromptRva);

  const size_t not_sil2 = code.size();
  EmitBytes(code, {0x41, 0x80, 0xBE, 0x8E, 0x01, 0x00, 0x00, 0x00});
  const size_t jne_skip = EmitJccRel32Placeholder(code, 0x85);
  EmitAbsJump(code, g_game + kOriginalContinueRva);

  const size_t skip_prompt = code.size();
  EmitAbsJump(code, g_game + kSkipPromptRva);

  PatchRel32(code, jne_not_sil2, not_sil2);
  PatchRel32(code, jne_skip, skip_prompt);
  return code;
}

std::vector<uint8_t> BuildPromptUpdateStub() {
  std::vector<uint8_t> code;
  code.reserve(160);

  EmitSaveVolatile(code);
  EmitMovRaxImm64(code, reinterpret_cast<uint64_t>(&RecordPromptUpdate));
  EmitBytes(code, {0xFF, 0xD0});
  EmitRestoreVolatile(code);

  // Original first 15 bytes at 0x140BD0C70. They are all prologue bytes with
  // no relative operands, so they are safe to replay before returning.
  EmitBytes(code, {0x48, 0x89, 0x54, 0x24, 0x10, 0x55, 0x53, 0x56,
                   0x57, 0x41, 0x54, 0x41, 0x55, 0x41, 0x56});
  EmitAbsJump(code, g_game + kPromptUpdateEntryRva + kPromptUpdatePatchLen);
  return code;
}

std::vector<uint8_t> BuildPromptTextAStub() {
  std::vector<uint8_t> code;
  code.reserve(220);

  EmitBytes(code, {0x41, 0x0F, 0xB6, 0x4D, 0x3A});  // movzx ecx,[r13+3Ah]
  EmitBytes(code, {0x49, 0x8B, 0x45, 0x30});        // mov rax,[r13+30h]
  EmitBytes(code, {0x4D, 0x8D, 0x86, 0x80, 0x01, 0x00,
                   0x00});                          // lea r8,[r14+180h]
  EmitBytes(code, {0x88, 0x4C, 0x24, 0x20});        // mov [rsp+20h],cl
  EmitMovR9Imm64(code, g_game + kPromptTextALiteralRva);
  EmitBytes(code, {0x48, 0x8B, 0x10});              // mov rdx,[rax]
  EmitBytes(code, {0x48, 0x8B, 0xCF});              // mov rcx,rdi

  EmitSaveVolatile(code);
  EmitBytes(code, {0x48, 0x89, 0xD1});              // rcx=text pointer
  EmitBytes(code, {0x4C, 0x89, 0xEA});              // rdx=r13 entry
  EmitBytes(code, {0x4C, 0x8B, 0xC7});              // r8=rdi panel
  EmitBytes(code, {0x4D, 0x89, 0xF1});              // r9=r14 owner/context
  EmitMovRaxImm64(code, reinterpret_cast<uint64_t>(&RecordPromptTextA));
  EmitBytes(code, {0xFF, 0xD0});
  EmitRestoreVolatile(code);

  EmitMovRaxImm64(code, g_game + kPromptTextACallRva);
  EmitBytes(code, {0xFF, 0xD0});
  EmitAbsJump(code, g_game + kPromptTextAReturnRva);
  return code;
}

std::vector<uint8_t> BuildPromptTextBStub() {
  std::vector<uint8_t> code;
  code.reserve(200);

  EmitBytes(code, {0x49, 0x8B, 0x45, 0x40});        // mov rax,[r13+40h]
  EmitBytes(code, {0x41, 0xB0, 0x01});              // mov r8b,1
  EmitBytes(code, {0x48, 0x8B, 0x10});              // mov rdx,[rax]
  EmitBytes(code, {0x48, 0x8B, 0xCF});              // mov rcx,rdi

  EmitSaveVolatile(code);
  EmitBytes(code, {0x48, 0x89, 0xD1});              // rcx=text pointer
  EmitBytes(code, {0x4C, 0x89, 0xEA});              // rdx=r13 entry
  EmitBytes(code, {0x4C, 0x8B, 0xC7});              // r8=rdi panel
  EmitBytes(code, {0x4D, 0x89, 0xF1});              // r9=r14 owner/context
  EmitMovRaxImm64(code, reinterpret_cast<uint64_t>(&RecordPromptTextB));
  EmitBytes(code, {0xFF, 0xD0});
  EmitRestoreVolatile(code);

  EmitMovRaxImm64(code, g_game + kPromptTextBCallRva);
  EmitBytes(code, {0xFF, 0xD0});
  EmitAbsJump(code, g_game + kPromptTextBReturnRva);
  return code;
}

bool InstallAbsJumpHook(uint8_t* target, const uint8_t* expected,
                        size_t expected_size, const std::vector<uint8_t>& stub,
                        size_t patch_len, const char* name, void** stub_mem) {
  if (patch_len < 14) return false;
  if (std::memcmp(target, expected, expected_size) != 0) {
    Log("%s mismatch: target=%p first=%02X %02X %02X %02X", name, target,
        target[0], target[1], target[2], target[3]);
    return false;
  }

  void* mem = VirtualAlloc(nullptr, stub.size(), MEM_COMMIT | MEM_RESERVE,
                           PAGE_EXECUTE_READWRITE);
  if (!mem) {
    Log("%s VirtualAlloc failed: %lu", name, GetLastError());
    return false;
  }
  std::memcpy(mem, stub.data(), stub.size());

  std::vector<uint8_t> patch(patch_len, 0x90);
  patch[0] = 0xFF;
  patch[1] = 0x25;
  *reinterpret_cast<uint32_t*>(patch.data() + 2) = 0;
  *reinterpret_cast<uint64_t*>(patch.data() + 6) =
      reinterpret_cast<uint64_t>(mem);

  DWORD old_protect = 0;
  if (!VirtualProtect(target, patch.size(), PAGE_EXECUTE_READWRITE,
                      &old_protect)) {
    Log("%s VirtualProtect failed: %lu", name, GetLastError());
    return false;
  }
  std::memcpy(target, patch.data(), patch.size());
  DWORD unused = 0;
  VirtualProtect(target, patch.size(), old_protect, &unused);
  FlushInstructionCache(GetCurrentProcess(), target, patch.size());
  FlushInstructionCache(GetCurrentProcess(), mem, stub.size());
  if (stub_mem) *stub_mem = mem;
  Log("%s installed: target=%p stub=%p size=%zu", name, target, mem,
      stub.size());
  return true;
}

bool InstallPromptTextHooks() {
  uint8_t* target_a = reinterpret_cast<uint8_t*>(g_game + kPromptTextAEntryRva);
  const uint8_t expected_a[] = {
      0x41, 0x0F, 0xB6, 0x4D, 0x3A, 0x49, 0x8B, 0x45,
      0x30, 0x4D, 0x8D, 0x86, 0x80, 0x01, 0x00, 0x00,
      0x88, 0x4C, 0x24, 0x20, 0x4C, 0x8D, 0x0D, 0x3E,
      0x04, 0xE8, 0x03, 0x48, 0x8B, 0x10, 0x48, 0x8B,
      0xCF, 0xE8, 0x23, 0x3A, 0xEF, 0xFF};
  void* stub_a = nullptr;
  const bool ok_a = InstallAbsJumpHook(
      target_a, expected_a, sizeof(expected_a), BuildPromptTextAStub(),
      kPromptTextAPatchLen, "prompt text A hook", &stub_a);

  uint8_t* target_b = reinterpret_cast<uint8_t*>(g_game + kPromptTextBEntryRva);
  const uint8_t expected_b[] = {0x49, 0x8B, 0x45, 0x40, 0x41, 0xB0,
                                0x01, 0x48, 0x8B, 0x10, 0x48, 0x8B,
                                0xCF, 0xE8, 0x8F, 0x34, 0xEF, 0xFF};
  void* stub_b = nullptr;
  const bool ok_b = InstallAbsJumpHook(
      target_b, expected_b, sizeof(expected_b), BuildPromptTextBStub(),
      kPromptTextBPatchLen, "prompt text B hook", &stub_b);

  if (ok_a || ok_b) InterlockedExchange(&g_prompt_text_hook_installed, 1);
  return ok_a || ok_b;
}

bool InstallPromptUpdateHook() {
  uint8_t* target = reinterpret_cast<uint8_t*>(g_game + kPromptUpdateEntryRva);
  const uint8_t expected[] = {0x48, 0x89, 0x54, 0x24, 0x10,
                              0x55, 0x53, 0x56, 0x57,
                              0x41, 0x54, 0x41, 0x55,
                              0x41, 0x56};
  if (std::memcmp(target, expected, sizeof(expected)) != 0) {
    Log("prompt update hook mismatch: target=%p first=%02X %02X %02X %02X",
        target, target[0], target[1], target[2], target[3]);
    return false;
  }

  std::vector<uint8_t> stub = BuildPromptUpdateStub();
  void* mem = VirtualAlloc(nullptr, stub.size(), MEM_COMMIT | MEM_RESERVE,
                           PAGE_EXECUTE_READWRITE);
  if (!mem) {
    Log("prompt update VirtualAlloc failed: %lu", GetLastError());
    return false;
  }
  std::memcpy(mem, stub.data(), stub.size());

  uint8_t patch[kPromptUpdatePatchLen]{};
  patch[0] = 0xFF;
  patch[1] = 0x25;
  *reinterpret_cast<uint32_t*>(patch + 2) = 0;
  *reinterpret_cast<uint64_t*>(patch + 6) = reinterpret_cast<uint64_t>(mem);
  for (size_t i = 14; i < sizeof(patch); ++i) patch[i] = 0x90;

  DWORD old_protect = 0;
  if (!VirtualProtect(target, sizeof(patch), PAGE_EXECUTE_READWRITE,
                      &old_protect)) {
    Log("prompt update VirtualProtect failed: %lu", GetLastError());
    return false;
  }
  std::memcpy(target, patch, sizeof(patch));
  DWORD unused = 0;
  VirtualProtect(target, sizeof(patch), old_protect, &unused);
  FlushInstructionCache(GetCurrentProcess(), target, sizeof(patch));
  FlushInstructionCache(GetCurrentProcess(), mem, stub.size());
  InterlockedExchange(&g_prompt_hook_installed, 1);
  Log("prompt update hook installed: target=%p stub=%p size=%zu", target, mem,
      stub.size());
  return true;
}

bool InstallPromptBranchHook() {
  uint8_t* target = reinterpret_cast<uint8_t*>(g_game + kPromptBranchRva);
  const uint8_t expected[] = {
      0x40, 0x80, 0xFE, 0x02, 0x0F, 0x84, 0xF1, 0x00,
      0x00, 0x00, 0x41, 0x80, 0xBE, 0x8E, 0x01, 0x00,
      0x00, 0x00, 0x0F, 0x85, 0xE3, 0x00, 0x00};
  if (std::memcmp(target, expected, sizeof(expected)) != 0) {
    Log("hook point mismatch: target=%p first=%02X %02X %02X %02X",
        target, target[0], target[1], target[2], target[3]);
    return false;
  }

  std::vector<uint8_t> stub = BuildPromptBranchStub();
  void* mem = VirtualAlloc(nullptr, stub.size(), MEM_COMMIT | MEM_RESERVE,
                           PAGE_EXECUTE_READWRITE);
  if (!mem) {
    Log("VirtualAlloc failed: %lu", GetLastError());
    return false;
  }
  std::memcpy(mem, stub.data(), stub.size());

  uint8_t patch[kPatchLen]{};
  patch[0] = 0xFF;
  patch[1] = 0x25;
  *reinterpret_cast<uint32_t*>(patch + 2) = 0;
  *reinterpret_cast<uint64_t*>(patch + 6) = reinterpret_cast<uint64_t>(mem);
  for (size_t i = 14; i < sizeof(patch); ++i) patch[i] = 0x90;

  DWORD old_protect = 0;
  if (!VirtualProtect(target, sizeof(patch), PAGE_EXECUTE_READWRITE,
                      &old_protect)) {
    Log("VirtualProtect failed: %lu", GetLastError());
    return false;
  }
  std::memcpy(target, patch, sizeof(patch));
  DWORD unused = 0;
  VirtualProtect(target, sizeof(patch), old_protect, &unused);
  FlushInstructionCache(GetCurrentProcess(), target, sizeof(patch));
  FlushInstructionCache(GetCurrentProcess(), mem, stub.size());
  InterlockedExchange(&g_installed, 1);
  Log("hook installed: target=%p stub=%p size=%zu", target, mem, stub.size());
  return true;
}

bool LoadItemDatabase() {
  g_items.clear();
  g_item_names.clear();
  g_item_name_lookup.clear();

  FILE* f = nullptr;
  _wfopen_s(&f, g_item_db_path.c_str(), L"rb");
  if (!f) {
    Log("item database missing: %S", g_item_db_path.c_str());
    return false;
  }

  char line[2048]{};
  while (std::fgets(line, sizeof(line), f)) {
    if (line[0] == '#' || line[0] == '\r' || line[0] == '\n' || line[0] == 0) {
      continue;
    }
    char* fields[8]{};
    int field_count = 0;
    char* cursor = line;
    while (field_count < 8 && cursor) {
      fields[field_count++] = cursor;
      char* tab = std::strchr(cursor, '\t');
      if (!tab) break;
      *tab = 0;
      cursor = tab + 1;
    }
    for (int i = 0; i < field_count; ++i) TrimAsciiInPlace(fields[i]);
    if (field_count < 2) continue;
    const uint32_t key = static_cast<uint32_t>(std::strtoul(fields[0], nullptr, 10));
    const uint8_t category = ParseCategory(fields[1]);
    if (key == 0 || category == kCatUnknown) continue;
    ItemInfo info{};
    info.key = key;
    info.category = category;
    if (field_count > 2) info.zh = Utf8ToWide(fields[2]);
    if (field_count > 3) info.en = Utf8ToWide(fields[3]);
    if (field_count > 4) info.internal = Utf8ToWide(fields[4]);
    g_items.push_back(info);
    AddItemNameRef(info.zh, key, category);
    AddItemNameRef(info.en, key, category);
    AddItemNameRef(info.internal, key, category);
  }
  std::fclose(f);

  std::sort(g_items.begin(), g_items.end(),
            [](const ItemInfo& a, const ItemInfo& b) { return a.key < b.key; });
  g_items.erase(std::unique(g_items.begin(), g_items.end(),
                            [](const ItemInfo& a, const ItemInfo& b) {
                              return a.key == b.key;
                            }),
                g_items.end());
  std::sort(g_item_names.begin(), g_item_names.end(),
            [](const ItemNameRef& a, const ItemNameRef& b) {
              if (a.name.size() != b.name.size()) {
                return a.name.size() > b.name.size();
              }
              return a.key < b.key;
            });
  g_item_names.erase(std::unique(g_item_names.begin(), g_item_names.end(),
                                 [](const ItemNameRef& a,
                                    const ItemNameRef& b) {
                                   return a.name == b.name && a.key == b.key;
                                 }),
                     g_item_names.end());
  for (const ItemNameRef& ref : g_item_names) {
    if (!ref.name.empty() && g_item_name_lookup.find(ref.name) ==
                                 g_item_name_lookup.end()) {
      g_item_name_lookup.emplace(ref.name, ref);
    }
  }
  Log("item database loaded: path=%S items=%zu names=%zu",
      g_item_db_path.c_str(), g_items.size(), g_item_names.size());
  return !g_items.empty();
}

void LogChangedCounters(std::array<LONG64, 1024>& last_seen,
                        std::array<LONG64, 1024>& last_triggered,
                        std::array<LONG64, 1024>& last_filtered) {
  for (size_t i = 0; i < g_seen.size(); ++i) {
    const LONG64 seen = InterlockedCompareExchange64(&g_seen[i], 0, 0);
    const LONG64 triggered =
        InterlockedCompareExchange64(&g_triggered[i], 0, 0);
    const LONG64 filtered =
        InterlockedCompareExchange64(&g_filtered[i], 0, 0);
    if (seen == last_seen[i] && triggered == last_triggered[i] &&
        filtered == last_filtered[i]) {
      continue;
    }

    if (IsGroundLootType(static_cast<uint32_t>(i))) {
      const LONG category =
          InterlockedCompareExchange(&g_last_ground_item_category, 0, 0);
      Log("type %zu ground seen=%lld trigger=%lld filtered=%lld context=%p item=%ld category=%s allowed=%ld blocked=%ld text=%ld ambiguous=%ld unique=%ld confirmed=%ld source=%ld offset=0x%lX target=%p candidate=%p",
          i, seen, triggered, filtered,
          reinterpret_cast<void*>(
              InterlockedCompareExchange64(&g_last_interaction_context, 0, 0)),
          InterlockedCompareExchange(&g_last_ground_item_key, 0, 0),
          CategoryName(static_cast<uint8_t>(category)),
          InterlockedCompareExchange(&g_last_ground_item_allowed, 0, 0),
          InterlockedCompareExchange(&g_last_ground_item_blocked, 0, 0),
          InterlockedCompareExchange(&g_last_ground_item_text_match, 0, 0),
          InterlockedCompareExchange(&g_last_ground_item_ambiguous, 0, 0),
          InterlockedCompareExchange(&g_last_ground_item_unique_keys, 0, 0),
          InterlockedCompareExchange(&g_last_ground_item_confirmed, 0, 0),
          InterlockedCompareExchange(&g_last_ground_item_source, 0, 0),
          InterlockedCompareExchange(&g_last_ground_item_offset, 0, 0),
          reinterpret_cast<void*>(
              InterlockedCompareExchange64(&g_last_ground_target, 0, 0)),
          reinterpret_cast<void*>(
              InterlockedCompareExchange64(&g_last_ground_candidate, 0, 0)));
    } else if (i == kCorpseLootType) {
      Log("type %zu corpse seen=%lld trigger=%lld filtered=%lld context=%p target=%p candidate=%p", i,
          seen, triggered,
          filtered,
          reinterpret_cast<void*>(
              InterlockedCompareExchange64(&g_last_interaction_context, 0, 0)),
          reinterpret_cast<void*>(
              InterlockedCompareExchange64(&g_last_corpse_target, 0, 0)),
          reinterpret_cast<void*>(
              InterlockedCompareExchange64(&g_last_corpse_candidate, 0, 0)));
    } else {
      Log("type %zu seen=%lld trigger=%lld", i, seen, triggered);
    }

    last_seen[i] = seen;
    last_triggered[i] = triggered;
    last_filtered[i] = filtered;
  }
}

bool KeyDown(int vk);

bool KeyDown(int vk) { return (GetAsyncKeyState(vk) & 0x8000) != 0; }

bool HotkeyDown(WORD vk, uint8_t mods) {
  if (vk == 0) return false;
  if ((mods & kHotkeyAlt) && !KeyDown(VK_MENU)) return false;
  if ((mods & kHotkeyCtrl) && !KeyDown(VK_CONTROL)) return false;
  if ((mods & kHotkeyShift) && !KeyDown(VK_SHIFT)) return false;
  return KeyDown(vk);
}

void HandleToggleHotkey() {
  static bool was_down = false;
  const WORD vk =
      static_cast<WORD>(InterlockedCompareExchange(&g_toggle_hotkey_vk, 0, 0));
  const uint8_t mods = static_cast<uint8_t>(
      InterlockedCompareExchange(&g_toggle_hotkey_mods, 0, 0));
  const bool down = HotkeyDown(vk, mods);
  if (down && !was_down) {
    const LONG old = InterlockedCompareExchange(&g_enabled, 0, 0);
    const LONG next = old ? 0 : 1;
    InterlockedExchange(&g_enabled, next);
    ShowStatusToast(next ? L"\u81ea\u52a8\u62fe\u53d6\uff1a\u5df2\u5f00\u542f"
                         : L"\u81ea\u52a8\u62fe\u53d6\uff1a\u5df2\u5173\u95ed");
    Log("toggle hotkey fired enabled=%ld vk=0x%X mods=0x%X", next, vk, mods);
  }
  was_down = down;
}

BOOL CALLBACK ConfigWindowEnumProc(HWND hwnd, LPARAM param) {
  wchar_t cls[128]{};
  GetClassNameW(hwnd, cls, static_cast<int>(sizeof(cls) / sizeof(cls[0])));
  if (std::wcscmp(cls, L"CrimsonAutolootConfigWindow") == 0) {
    PostMessageW(hwnd, WM_CLOSE, 0, 0);
    ++(*reinterpret_cast<int*>(param));
  }
  return TRUE;
}

int CloseConfigWindows() {
  int count = 0;
  EnumWindows(ConfigWindowEnumProc, reinterpret_cast<LPARAM>(&count));
  return count;
}

void ToggleConfigWindow() {
  const int closed = CloseConfigWindows();
  if (closed > 0) {
    ShowStatusToast(L"\u914d\u7f6e\u7a97\u53e3\uff1a\u5df2\u5173\u95ed");
    Log("hotkey Alt+P close config windows=%d", closed);
    return;
  }

  HINSTANCE result =
      ShellExecuteW(nullptr, L"open", g_config_exe_path.c_str(), nullptr,
                    g_support_dir.c_str(), SW_SHOWNORMAL);
  ShowStatusToast(L"\u914d\u7f6e\u7a97\u53e3\uff1a\u5df2\u6253\u5f00");
  Log("hotkey Alt+P launch config result=%p path=%S", result,
      g_config_exe_path.c_str());
}

void HandleConfigHotkey() {
  static bool was_down = false;
  const WORD vk =
      static_cast<WORD>(InterlockedCompareExchange(&g_config_hotkey_vk, 0, 0));
  const uint8_t mods = static_cast<uint8_t>(
      InterlockedCompareExchange(&g_config_hotkey_mods, 0, 0));
  const bool down = HotkeyDown(vk, mods);
  if (down && !was_down) {
    ToggleConfigWindow();
  }
  was_down = down;
}

void ProcessPendingInput(ULONGLONG& last_press) {
  if (InterlockedCompareExchange(&g_enabled, 0, 0) == 0) {
    InterlockedExchange(&g_pending_ground, 0);
    InterlockedExchange(&g_pending_corpse, 0);
    InterlockedExchange64(&g_pending_ground_tick, 0);
    InterlockedExchange64(&g_pending_corpse_tick, 0);
    return;
  }

  bool has_ground = InterlockedCompareExchange(&g_pending_ground, 0, 0) != 0;
  bool has_corpse = InterlockedCompareExchange(&g_pending_corpse, 0, 0) != 0;
  if (!has_ground && !has_corpse) return;

  const ULONGLONG now = GetTickCount64();
  const LONG interval = InterlockedCompareExchange(&g_trigger_interval_ms, 0, 0);
  ULONGLONG max_pending_age = static_cast<ULONGLONG>(interval) + 150;
  if (max_pending_age < kPendingInputMaxAgeMs) {
    max_pending_age = kPendingInputMaxAgeMs;
  }
  const ULONGLONG ground_tick = static_cast<ULONGLONG>(
      InterlockedCompareExchange64(&g_pending_ground_tick, 0, 0));
  const ULONGLONG corpse_tick = static_cast<ULONGLONG>(
      InterlockedCompareExchange64(&g_pending_corpse_tick, 0, 0));
  if (has_ground &&
      (ground_tick == 0 || now - ground_tick > max_pending_age)) {
    InterlockedExchange(&g_pending_ground, 0);
    InterlockedExchange64(&g_pending_ground_tick, 0);
    has_ground = false;
  }
  if (has_corpse &&
      (corpse_tick == 0 || now - corpse_tick > max_pending_age)) {
    InterlockedExchange(&g_pending_corpse, 0);
    InterlockedExchange64(&g_pending_corpse_tick, 0);
    has_corpse = false;
  }
  if (!has_ground && !has_corpse) return;

  if (now - last_press < static_cast<ULONGLONG>(interval)) return;
  if (!IsGameForeground()) return;

  InterlockedExchange(&g_pending_ground, 0);
  InterlockedExchange(&g_pending_corpse, 0);
  PressInteractKey();
  last_press = now;
  Log("sent interact key: ground=%d corpse=%d", has_ground ? 1 : 0,
      has_corpse ? 1 : 0);
}

DWORD WINAPI WorkerThread(void*) {
  g_single_instance =
      CreateMutexW(nullptr, TRUE, L"Local\\CrimsonAutolootCnRewriteOnce");
  if (!g_single_instance || GetLastError() == ERROR_ALREADY_EXISTS) {
    return 0;
  }

  g_dir = ModuleDirectory(g_self);
  g_support_dir = g_dir + L"\\crimson_autoloot_cn";
  CreateDirectoryW(g_support_dir.c_str(), nullptr);
  g_ini_path = g_support_dir + L"\\crimson_autoloot_cn.ini";
  g_default_ini_path = g_support_dir + L"\\crimson_autoloot_defaults.ini";
  g_log_path = g_support_dir + L"\\crimson_autoloot_cn.log";
  g_item_db_path = g_support_dir + L"\\crimson_autoloot_items.tsv";
  g_config_exe_path = g_support_dir + L"\\crimson_autoloot_config.exe";
  Log("startup rewrite plugin");

  HMODULE game = GetModuleHandleW(nullptr);
  if (!game) {
    Log("game module not found");
    return 0;
  }
  g_game = reinterpret_cast<uintptr_t>(game);
  const uint32_t timestamp = ReadMainModuleTimestamp(game);
  const bool version_ok = timestamp == kSupportedBuildTimestamp;
  InterlockedExchange(&g_version_ok, version_ok ? 1 : 0);

  LoadConfig();
  LoadItemDatabase();
  Log("game base=%p timestamp=0x%08X version_ok=%d strict=%ld",
      reinterpret_cast<void*>(g_game), timestamp, version_ok ? 1 : 0,
      InterlockedCompareExchange(&g_strict_version, 0, 0));
  Log("config enabled=%ld ground=%ld corpse=%ld filter=%ld blocked_items=%zu interval=%ld key=%c foreground_only=%ld",
      InterlockedCompareExchange(&g_enabled, 0, 0),
      InterlockedCompareExchange(&g_ground_enabled, 0, 0),
      InterlockedCompareExchange(&g_corpse_enabled, 0, 0),
      InterlockedCompareExchange(&g_item_filter_enabled, 0, 0),
      BlockedItemCount(),
      InterlockedCompareExchange(&g_trigger_interval_ms, 0, 0),
      static_cast<char>(InterlockedCompareExchange(&g_interact_key, 0, 0)),
      InterlockedCompareExchange(&g_game_foreground_only, 0, 0));

  Sleep(2500);
  InstallPromptTextHooks();
  Log("prompt update hook disabled: using stable branch hook only");
  InstallPromptBranchHook();

  ULONGLONG last_config_check = GetTickCount64();
  ULONGLONG last_config_write_time = ConfigWriteTime();
  ULONGLONG last_counter_log = GetTickCount64();
  ULONGLONG last_press = 0;
  std::array<LONG64, 1024> last_seen{};
  std::array<LONG64, 1024> last_triggered{};
  std::array<LONG64, 1024> last_filtered{};

  for (;;) {
    Sleep(25);
    PumpStatusMessages();
    HandleToggleHotkey();
    HandleConfigHotkey();
    const ULONGLONG now = GetTickCount64();
    if (InterlockedCompareExchange(&g_enabled, 0, 0) != 0) {
      ProcessPendingInput(last_press);
    } else {
      InterlockedExchange(&g_pending_ground, 0);
      InterlockedExchange(&g_pending_corpse, 0);
      InterlockedExchange64(&g_pending_ground_tick, 0);
      InterlockedExchange64(&g_pending_corpse_tick, 0);
    }

    UpdateStatusToast(now);
    if (now - last_config_check >= 500) {
      const ULONGLONG write_time = ConfigWriteTime();
      if (write_time != 0 && write_time != last_config_write_time) {
        LoadConfig();
        last_config_write_time = write_time;
        Log("config reloaded: filter=%ld blocked_items=%zu",
            InterlockedCompareExchange(&g_item_filter_enabled, 0, 0),
            BlockedItemCount());
      }
      last_config_check = now;
    }
    if (now - last_counter_log >= 2000) {
      LogChangedCounters(last_seen, last_triggered, last_filtered);
      last_counter_log = now;
    }
  }
}

}  // namespace

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
  if (reason == DLL_PROCESS_ATTACH) {
    g_self = module;
    DisableThreadLibraryCalls(module);
    HANDLE thread = CreateThread(nullptr, 0, WorkerThread, nullptr, 0, nullptr);
    if (thread) CloseHandle(thread);
  }
  return TRUE;
}
