param(
    [Parameter(Mandatory = $true)][string]$Version,
    [string]$BuildDirectory = 'build/windows-msvc-release'
)
$ErrorActionPreference = 'Stop'
if ($Version -notmatch '^\d+\.\d+\.\d+$') { throw '无效版本号' }

$build = (Resolve-Path $BuildDirectory).Path
$dist = Join-Path $build 'dist'
$artifacts = Join-Path $build 'artifacts'
New-Item -ItemType Directory -Force $artifacts | Out-Null
$name = "ZzApkInfo-$Version-windows-x64.zip"
$archive = Join-Path $artifacts $name
Compress-Archive -Path (Join-Path $dist '*') -DestinationPath $archive -Force

# 检查真正交付的 ZIP；带空格的解压路径可发现路径拼接错误。
$check = Join-Path $build 'package check'
if (Test-Path $check) { Remove-Item -Recurse -Force $check }
Expand-Archive -Path $archive -DestinationPath $check
foreach ($file in @('ZzApkInfo.exe', 'aapt.exe', 'aapt2.exe', 'Qt6Core.dll',
    'Qt6Gui.dll', 'Qt6Widgets.dll', 'Qt6Svg.dll', 'platforms/qwindows.dll',
    'msvcp140.dll', 'vcruntime140.dll', 'vcruntime140_1.dll', 'build-info.txt')) {
    if (-not (Test-Path (Join-Path $check $file))) { throw "包内文件缺失：$file" }
}

# 清除 SDK 搜索路径，从无关目录启动解压包，防止漏带运行库被环境掩盖。
$env:PATH = "$env:SystemRoot\System32;$env:SystemRoot"
Remove-Item Env:QT_PLUGIN_PATH, Env:QT_QPA_PLATFORM_PLUGIN_PATH -ErrorAction SilentlyContinue
$process = Start-Process (Join-Path $check 'ZzApkInfo.exe') -ArgumentList '--smoke' `
    -WorkingDirectory $env:TEMP -PassThru
if (-not $process.WaitForExit(15000)) {
    $process.Kill()
    throw '解压包启动超时'
}
if ($process.ExitCode -ne 0) { throw "解压包启动失败：$($process.ExitCode)" }
$digest = (Get-FileHash -Algorithm SHA256 $archive).Hash.ToLowerInvariant()
Set-Content -LiteralPath "$archive.sha256" -Value "$digest  $name" -Encoding utf8NoBOM
