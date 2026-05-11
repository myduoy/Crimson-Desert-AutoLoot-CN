# 安装、运行环境和卸载说明

本文档面向普通用户，说明 Crimson Desert AutoLoot CN 的运行环境、安装步骤、更新方法、常见问题和卸载方法。

## 运行环境

- 游戏：Steam 版 Crimson Desert，Windows x64 客户端。
- 插件目录：游戏安装目录下的 `bin64` 目录。
- 系统：Windows 10 / Windows 11 64 位。
- ASI 加载器：需要一个能加载 `.asi` 插件的 ASI Loader。
- 默认交互键：游戏内“基本互动”按键默认为 `E`，插件配置里的 `Interact key` 需要和游戏内按键一致。

推荐使用 Ultimate ASI Loader v9.7.1：

```text
https://github.com/ThirteenAG/Ultimate-ASI-Loader/releases/tag/v9.7.1
```

该加载器发布页说明：把 DLL 放进游戏目录即可加载 ASI，ASI 文件可放在游戏根目录或 `scripts`、`plugins`、`update` 文件夹中；它支持 `version.dll` 等多种代理 DLL 名称。本插件按 Crimson Desert 的 `bin64` 目录安装方式说明。

## 下载文件

从 GitHub Releases 或 Nexus Mods 下载发布包，例如：

```text
Crimson-Desert-AutoLoot-CN-v0.1.9.zip
```

解压后应能看到类似结构：

```text
Crimson-Desert-AutoLoot-CN\
  crimson_autoloot_cn.asi
  crimson_autoloot_cn\
    crimson_autoloot_config.exe
    crimson_autoloot_defaults.ini
    crimson_autoloot_items.tsv
  docs\
    INSTALL_UNINSTALL.en-US.md
    INSTALL_UNINSTALL.zh-CN.md
  README.md
  LICENSE
```

## 安装 ASI Loader

如果你已经安装过可用的 ASI Loader，可以跳过本节。

1. 关闭游戏。
2. 打开 Ultimate ASI Loader v9.7.1 发布页：

   ```text
   https://github.com/ThirteenAG/Ultimate-ASI-Loader/releases/tag/v9.7.1
   ```

3. 下载 64 位 DLL。Crimson Desert 是 64 位游戏，请不要使用 32 位 DLL。
4. 将下载到的 DLL 放入：

   ```text
   <Crimson Desert>\bin64\
   ```

5. 如果 DLL 文件名不是 `version.dll`，可按加载器说明改名为 `version.dll`。本项目当前推荐放在 `bin64\version.dll`。
6. 如果目录里已经有同名 `version.dll`，先备份旧文件，例如改名为：

   ```text
   version.dll.bak
   ```

注意：不要同时放多个不同来源、同名或互相冲突的 ASI Loader。多个代理 DLL 可能导致游戏无法启动或插件重复加载。

## 安装 AutoLoot 插件

1. 关闭游戏。
2. 解压 `Crimson-Desert-AutoLoot-CN-v0.1.9.zip`。
3. 将 `crimson_autoloot_cn.asi` 复制到：

   ```text
   <Crimson Desert>\bin64\crimson_autoloot_cn.asi
   ```

4. 将整个 `crimson_autoloot_cn` 文件夹复制到：

   ```text
   <Crimson Desert>\bin64\crimson_autoloot_cn\
   ```

5. 确认最终目录类似：

   ```text
   <Crimson Desert>\bin64\
     version.dll
     crimson_autoloot_cn.asi
     crimson_autoloot_cn\
       crimson_autoloot_config.exe
       crimson_autoloot_defaults.ini
       crimson_autoloot_items.tsv
   ```

6. 启动游戏。

## 首次运行

插件首次运行时会在支持目录生成实际配置文件：

```text
<Crimson Desert>\bin64\crimson_autoloot_cn\crimson_autoloot_cn.ini
```

默认热键：

```text
F9   开关自动拾取
F10  打开或关闭配置窗口
E    交互键，需要和游戏内“基本互动”按键一致
```

如果你的游戏内互动键不是 `E`，打开配置窗口后修改 `Interact key`，保存即可。插件会自动热重载配置，一般不需要重启游戏。

## 配置说明

可以通过两种方式打开配置：

- 游戏中按 `F10`。
- 退出游戏后运行：

  ```text
  <Crimson Desert>\bin64\crimson_autoloot_cn\crimson_autoloot_config.exe
  ```

主要选项：

- `Enable` / `启用`：总开关。
- `Ground` / `地面`：自动拾取地面物品。
- `Corpse` / `摸尸`：自动翻找尸体。
- `Filter` / `过滤`：启用物品分类和单物品过滤。
- `Only when game is foreground` / `只在游戏前台时触发`：游戏窗口不在前台时不发送交互键。
- `Interact` / `交互键`：游戏内互动键。
- `Toggle` / `开关键`：插件总开关热键。
- `Panel` / `面板键`：配置窗口热键。
- `Language` / `语言`：`Auto`、`Chinese`、`English`。

过滤规则：

- 左侧分类打勾表示允许拾取该分类。
- 左侧分类取消勾选会阻止该分类下全部物品。
- 右侧单个物品取消勾选会加入单物品屏蔽列表。
- 搜索框只过滤当前分类显示，不会改变勾选状态。

## 更新插件

1. 关闭游戏。
2. 备份旧配置文件：

   ```text
   <Crimson Desert>\bin64\crimson_autoloot_cn\crimson_autoloot_cn.ini
   ```

3. 用新发布包里的文件覆盖：

   ```text
   <Crimson Desert>\bin64\crimson_autoloot_cn.asi
   <Crimson Desert>\bin64\crimson_autoloot_cn\crimson_autoloot_config.exe
   <Crimson Desert>\bin64\crimson_autoloot_cn\crimson_autoloot_defaults.ini
   <Crimson Desert>\bin64\crimson_autoloot_cn\crimson_autoloot_items.tsv
   ```

4. 通常不要删除旧的 `crimson_autoloot_cn.ini`，这样可以保留你的热键、分类和物品过滤设置。
5. 如果更新后配置异常，可关闭游戏，临时移走旧 `crimson_autoloot_cn.ini`，让插件重新生成默认配置。

## 日志和排查

日志路径：

```text
<Crimson Desert>\bin64\crimson_autoloot_cn\crimson_autoloot_cn.log
```

默认不写详细日志。如需排查，打开配置窗口勾选 `Debug`，或在 INI 中设置：

```ini
[General]
DebugLog=1
```

常见问题：

- 游戏启动不了：先移走 `crimson_autoloot_cn.asi` 测试是否恢复，再检查 ASI Loader 是否放错版本或放了多个冲突加载器。
- F9/F10 没反应：检查配置里的 `Toggle key`、`Panel key` 是否和游戏内按键冲突。
- 不拾取物品：确认 `Enable`、`Ground`、`Filter` 设置；检查分类或单物品是否被取消勾选。
- 不摸尸：确认 `Corpse` 已开启，并且游戏画面中出现“翻找/搜索/摸尸”等交互提示。
- 英文系统仍显示中文：在配置窗口把 `Language` 改为 `English` 并保存。
- 游戏更新后失效：游戏更新可能改变 hook 地址，需要等待插件更新。

## 卸载插件

1. 关闭游戏。
2. 删除插件文件：

   ```text
   <Crimson Desert>\bin64\crimson_autoloot_cn.asi
   ```

3. 删除插件支持目录：

   ```text
   <Crimson Desert>\bin64\crimson_autoloot_cn\
   ```

4. 如果你只为本插件安装了 ASI Loader，也可以删除：

   ```text
   <Crimson Desert>\bin64\version.dll
   ```

5. 如果其他 mod 也依赖同一个 ASI Loader，不要删除 `version.dll`，否则其他 ASI 插件也会失效。
6. 如果之前备份过游戏原始 DLL，例如 `version.dll.bak`，需要按你自己的备份情况恢复。

## 完整清理

如需完全清理本插件留下的配置和日志，删除：

```text
<Crimson Desert>\bin64\crimson_autoloot_cn\crimson_autoloot_cn.ini
<Crimson Desert>\bin64\crimson_autoloot_cn\crimson_autoloot_cn.log
```

删除支持目录时这些文件会一起被删除。

## 安全说明

- 本插件是非官方社区插件，不包含游戏文件，也不隶属于 Pearl Abyss。
- 安装前建议备份 `bin64` 目录中被替换或新增的 DLL。
- 不建议在游戏运行时覆盖 `.asi`、`.dll` 或配置程序文件。
- 如果游戏更新后出现闪退，先移走 `.asi` 文件验证游戏本体是否正常。
