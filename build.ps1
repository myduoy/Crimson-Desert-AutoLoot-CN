$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
$Build = Join-Path $Root "build"
New-Item -ItemType Directory -Path $Build -Force | Out-Null

$VcSetupCandidates = @(
  $env:VCVARS64,
  "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat",
  "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat",
  "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat",
  "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
) | Where-Object { $_ }

$VcSetup = $VcSetupCandidates | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
if (-not $VcSetup) {
  throw "VC environment script not found. Install Visual Studio Build Tools, or set VCVARS64 to the full path of vcvars64.bat."
}

$Main = Join-Path $Root "src\main.cpp"
$Ui = Join-Path $Root "src\config_ui.cpp"
$Asi = Join-Path $Build "crimson_autoloot_cn.asi"
$ConfigExe = Join-Path $Build "crimson_autoloot_config.exe"

cmd.exe /c "call `"$VcSetup`" >nul && cl /nologo /std:c++17 /O2 /EHsc /LD /DUNICODE /D_UNICODE /Fe:`"$Asi`" `"$Main`" user32.lib shell32.lib gdi32.lib"
cmd.exe /c "call `"$VcSetup`" >nul && cl /nologo /std:c++17 /O2 /EHsc /DUNICODE /D_UNICODE /Fe:`"$ConfigExe`" `"$Ui`" user32.lib shell32.lib gdi32.lib comctl32.lib"

Write-Host "Built:"
Write-Host "  $Asi"
Write-Host "  $ConfigExe"
