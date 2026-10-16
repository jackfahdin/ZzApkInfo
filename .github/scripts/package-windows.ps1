#requires -Version 5.1
param(
    [string]$BuildDirectory = 'build/windows-msvc-release'
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
if ($env:OS -ne 'Windows_NT') { throw '此脚本仅支持 Windows。' }

# 路径以仓库为基准，允许从任意工作目录调用脚本。
$repository = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
if (-not [IO.Path]::IsPathRooted($BuildDirectory)) {
    $BuildDirectory = Join-Path $repository $BuildDirectory
}
$build = (Resolve-Path -LiteralPath $BuildDirectory).Path
if (-not (Test-Path -LiteralPath (Join-Path $build 'CMakeCache.txt'))) {
    throw '请先运行 cmake --preset windows-msvc-release 配置 Release 构建。'
}

# 构建目标内部调用 CPack，本地与 CI 共用相同依赖收集规则。
& cmake --build $build --config Release --target package-windows
if ($LASTEXITCODE -ne 0) { throw "构建或 CPack 打包失败：$LASTEXITCODE" }
$buildInfo = Get-Content -LiteralPath (Join-Path $build 'dist/build-info.txt')
if ($buildInfo[0] -notmatch '^ZzApkInfo ([0-9]+\.[0-9]+\.[0-9]+)$') { throw '无法读取包版本。' }
$name = "ZzApkInfo-$($Matches[1])-windows-x64"
$archive = Join-Path $repository "dist/$name.zip"

# 验证实际交付的 ZIP 及其顶层目录，解压路径包含空格。
$check = Join-Path $build 'package check'
if (Test-Path -LiteralPath $check) { Remove-Item -LiteralPath $check -Recurse -Force }
Expand-Archive -LiteralPath $archive -DestinationPath $check
$package = Join-Path $check $name
foreach ($file in @('ZzApkInfo.exe', 'aapt.exe', 'aapt2.exe', 'Qt6Core.dll',
    'Qt6Gui.dll', 'Qt6Widgets.dll', 'Qt6Svg.dll', 'platforms/qwindows.dll',
    'imageformats/qico.dll', 'qt.conf', 'build-info.txt')) {
    if (-not (Test-Path -LiteralPath (Join-Path $package $file))) { throw "包内文件缺失：$file" }
}
$digest = (Get-FileHash -Algorithm SHA256 -LiteralPath $archive).Hash.ToLowerInvariant()
$checksum = (Get-Content -LiteralPath "$archive.sha256" -Raw).TrimEnd()
if ($checksum -cne "$digest  $name.zip") { throw 'ZIP 校验文件不匹配。' }

# 启动时仅使用包内运行库和插件，结束后恢复调用者的环境变量。
$environmentNames = @('PATH', 'QT_PLUGIN_PATH', 'QT_QPA_PLATFORM_PLUGIN_PATH',
    'QT_QPA_PLATFORM', 'QT_STYLE_OVERRIDE')
$savedEnvironment = @{}
foreach ($key in $environmentNames) { $savedEnvironment[$key] = [Environment]::GetEnvironmentVariable($key, 'Process') }
try {
    $env:PATH = "$env:SystemRoot\System32;$env:SystemRoot"
    foreach ($key in $environmentNames | Where-Object { $_ -ne 'PATH' }) {
        [Environment]::SetEnvironmentVariable($key, $null, 'Process')
    }
    $process = Start-Process -FilePath (Join-Path $package 'ZzApkInfo.exe') `
        -ArgumentList '--smoke' -WorkingDirectory $env:TEMP -PassThru
    try {
        # 提前获取进程句柄，确保短命进程退出后仍可可靠读取 ExitCode。
        $null = $process.Handle
        if (-not $process.WaitForExit(15000)) {
            $process.Kill()
            throw '解压包启动超时。'
        }
        if ($process.ExitCode -ne 0) { throw "解压包启动失败：$($process.ExitCode)" }
    } finally {
        $process.Dispose()
    }
} finally {
    foreach ($key in $environmentNames) {
        [Environment]::SetEnvironmentVariable($key, $savedEnvironment[$key], 'Process')
    }
}
Write-Host "打包完成：$archive"
Write-Host "便携目录：$(Join-Path $repository "dist/$name")"
