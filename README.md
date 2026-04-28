# Crimson Desert AutoLoot CN

Crimson Desert AutoLoot CN 是一个 Windows x64 ASI 插件，用于给 Crimson Desert 添加自动拾取、自动摸尸和中文配置界面。

> 这是非官方社区插件。项目不包含游戏本体文件，不隶属于 Pearl Abyss 或 Crimson Desert 官方。

## 功能

- 自动地面拾取：识别可拾取物品提示后自动发送交互键。
- 自动摸尸：识别尸体/区域搜刮交互后自动发送交互键。
- 物品过滤：按物品分类控制是否拾取，并支持单个物品加入不拾取列表。
- 中文配置界面：可视化开关功能、设置热键、搜索物品、打开日志和物品表。
- 当前系统语言物品名：物品列表优先显示当前语言名称，未翻译时回退到英文或内部名。
- 热加载配置：配置文件保存后，游戏内插件会自动重新读取。
- 低卡顿过滤：地面物品的重文本解析在插件 worker 线程中处理，避免阻塞游戏 hook 回调。

## 当前版本说明

当前公开版本不包含“战斗中暂停自动拾取”功能。之前用于探测战斗状态、武器状态、UI 提示扫描的代码已经移除，避免误判和卡顿。

`v0.1.1` 起默认关闭调试日志，并减少同一地面物品的重复文本扫描。如果需要排查问题，可以在 `crimson_autoloot_cn.ini` 中把 `DebugLog=1` 打开。

## 文件说明

- `release/crimson_autoloot_cn.asi`：游戏内 ASI 插件。
- `release/crimson_autoloot_config.exe`：中文配置界面。
- `src/main.cpp`：ASI 插件源码。
- `src/config_ui.cpp`：配置界面源码。
- `crimson_autoloot_defaults.ini`：默认配置。
- `crimson_autoloot_items.tsv`：物品分类和名称表。
- `tools/generate_items.py`：物品表生成脚本。
- `build.ps1`：本地编译脚本。

## 安装

1. 关闭游戏。
2. 将 `release/crimson_autoloot_cn.asi` 复制到游戏目录：

   ```text
   <Crimson Desert>\bin64\crimson_autoloot_cn.asi
   ```

3. 在同目录创建或复制支持目录：

   ```text
   <Crimson Desert>\bin64\crimson_autoloot_cn\
   ```

4. 将以下文件放入支持目录：

   ```text
   crimson_autoloot_config.exe
   crimson_autoloot_defaults.ini
   crimson_autoloot_items.tsv
   ```

5. 启动游戏。默认热键：

   ```text
   F9  开关自动拾取
   F10 打开/关闭配置面板
   E   游戏交互键
   ```

## 配置

运行 `crimson_autoloot_config.exe` 或在游戏内按 `F10` 打开配置面板。

主要配置：

- `启用`：总开关。
- `地面`：是否自动拾取地面物品。
- `摸尸`：是否自动摸尸/搜刮。
- `过滤`：是否启用物品过滤。
- `只在游戏前台时触发`：游戏窗口不在前台时不发送按键。
- `交互键`：默认 `E`，应与游戏内“基本互动”按键一致。
- `开关键`：默认 `F9`。
- `面板键`：默认 `F10`。

物品过滤逻辑：

- 左侧分类勾选表示该分类允许拾取。
- 取消分类勾选会把该分类下物品全部设为不拾取。
- 在右侧物品列表取消单个物品勾选，会把该物品加入不拾取列表。
- 搜索框只显示当前分类下匹配名称的物品。

## 从源码编译

需要 Visual Studio Build Tools C++ 工具链。

```powershell
powershell -ExecutionPolicy Bypass -File .\build.ps1
```

编译结果输出到：

```text
build\crimson_autoloot_cn.asi
build\crimson_autoloot_config.exe
```

## 日志

运行时日志位于：

```text
<Crimson Desert>\bin64\crimson_autoloot_cn\crimson_autoloot_cn.log
```

默认 `DebugLog=0`，不会持续写入运行日志。排查问题时再临时改为 `DebugLog=1`。

如果自动拾取不生效，先确认：

- 游戏交互键是否仍为 `E`。
- 插件是否启用。
- 物品分类或单个物品是否被过滤。
- 游戏窗口是否在前台。

## 许可证

本项目以 MIT License 开源。
