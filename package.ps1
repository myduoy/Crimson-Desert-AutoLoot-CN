$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
$Dist = Join-Path $Root "dist"
$Stage = Join-Path $Dist "Crimson-Desert-AutoLoot-CN"
$Support = Join-Path $Stage "crimson_autoloot_cn"
$Zip = Join-Path $Dist "Crimson-Desert-AutoLoot-CN-v0.1.2.zip"

if (Test-Path -LiteralPath $Stage) {
  Remove-Item -LiteralPath $Stage -Recurse -Force
}
New-Item -ItemType Directory -Path $Support -Force | Out-Null

Copy-Item -LiteralPath (Join-Path $Root "release\crimson_autoloot_cn.asi") -Destination (Join-Path $Stage "crimson_autoloot_cn.asi") -Force
Copy-Item -LiteralPath (Join-Path $Root "release\crimson_autoloot_config.exe") -Destination (Join-Path $Support "crimson_autoloot_config.exe") -Force
Copy-Item -LiteralPath (Join-Path $Root "crimson_autoloot_defaults.ini") -Destination (Join-Path $Support "crimson_autoloot_defaults.ini") -Force
Copy-Item -LiteralPath (Join-Path $Root "crimson_autoloot_items.tsv") -Destination (Join-Path $Support "crimson_autoloot_items.tsv") -Force
Copy-Item -LiteralPath (Join-Path $Root "README.md") -Destination (Join-Path $Stage "README.md") -Force
Copy-Item -LiteralPath (Join-Path $Root "LICENSE") -Destination (Join-Path $Stage "LICENSE") -Force

if (Test-Path -LiteralPath $Zip) {
  Remove-Item -LiteralPath $Zip -Force
}
Compress-Archive -LiteralPath $Stage -DestinationPath $Zip -Force
Write-Host $Zip
