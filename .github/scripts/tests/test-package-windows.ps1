# 使用真实 ZIP/校验和操作，只替换构建命令及 Windows 进程启动边界。
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$root = Join-Path ([IO.Path]::GetTempPath()) ('apk packaging ' + [Guid]::NewGuid())
$originalLocation = Get-Location
$keys = @('OS', 'PATH', 'TEMP', 'SystemRoot', 'QT_PLUGIN_PATH', 'QT_QPA_PLATFORM_PLUGIN_PATH', 'QT_QPA_PLATFORM', 'QT_STYLE_OVERRIDE')
$environment = @{}
foreach ($key in $keys) { $environment[$key] = [Environment]::GetEnvironmentVariable($key, 'Process') }
function Assert-True($condition, $message) { if (-not $condition) { throw $message } }
function cmake {
    Assert-True ($args -contains 'package-windows') '应通过统一目标调用 CPack。'
    $global:LASTEXITCODE = 0
}
function Start-Process {
    param($FilePath, $ArgumentList, $WorkingDirectory, [switch]$PassThru)
    Assert-True ($FilePath.EndsWith('ZzApkInfo.exe')) '启动文件错误。'
    Assert-True ($ArgumentList -eq '--smoke') '缺少启动检查参数。'
    Assert-True ($env:PATH -eq "$env:SystemRoot\System32;$env:SystemRoot") '启动环境仍包含 SDK PATH。'
    Assert-True ([string]::IsNullOrEmpty($env:QT_QPA_PLATFORM)) '未清除平台覆盖配置。'
    $process = [pscustomobject]@{ Handle = 1; ExitCode = $global:ZZAPK_TEST_EXIT_CODE }
    $process | Add-Member ScriptMethod WaitForExit { param($milliseconds) return $true }
    $process | Add-Member ScriptMethod Dispose {}
    return $process
}
try {
    $scripts = Join-Path $root '.github/scripts'
    $build = Join-Path $root 'custom build'
    $dist = Join-Path $root 'dist'
    $null = New-Item -ItemType Directory -Path $scripts, (Join-Path $build 'dist'), $dist -Force
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot '../package-windows.ps1') -Destination $scripts
    Set-Content -LiteralPath (Join-Path $build 'CMakeCache.txt') -Value 'configured'
    Set-Content -LiteralPath (Join-Path $build 'dist/build-info.txt') -Value "ZzApkInfo 1.2.3`nSource test"
    $name = 'ZzApkInfo-1.2.3-windows-x64'
    $package = Join-Path $root $name
    foreach ($file in @('ZzApkInfo.exe', 'aapt.exe', 'aapt2.exe', 'Qt6Core.dll', 'Qt6Gui.dll',
        'Qt6Widgets.dll', 'Qt6Svg.dll', 'platforms/qwindows.dll', 'imageformats/qico.dll', 'qt.conf', 'build-info.txt')) {
        $path = Join-Path $package $file
        $null = New-Item -ItemType Directory -Path (Split-Path $path) -Force
        Set-Content -LiteralPath $path -Value $file
    }
    $archive = Join-Path $dist "$name.zip"
    Compress-Archive -LiteralPath $package -DestinationPath $archive
    $digest = (Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLowerInvariant()
    [IO.File]::WriteAllText("$archive.sha256", "$digest  $name.zip`n")
    $env:OS = 'Windows_NT'
    $env:TEMP = $root
    $env:SystemRoot = Join-Path $root 'Windows'
    $env:QT_QPA_PLATFORM = 'offscreen'
    $env:QT_PLUGIN_PATH = 'sentinel plugins'
    $savedPath = $env:PATH
    Set-Location ([IO.Path]::GetTempPath())
    foreach ($global:ZZAPK_TEST_EXIT_CODE in @(0, 7)) {
        $failure = $null
        try { & (Join-Path $scripts 'package-windows.ps1') -BuildDirectory 'custom build' }
        catch { $failure = $_ }
        Assert-True (($null -eq $failure) -eq ($global:ZZAPK_TEST_EXIT_CODE -eq 0)) "意外的脚本结果：$failure"
        Assert-True ($env:PATH -eq $savedPath) 'PATH 未恢复。'
        Assert-True ($env:QT_QPA_PLATFORM -eq 'offscreen') '平台变量未恢复。'
        Assert-True ($env:QT_PLUGIN_PATH -eq 'sentinel plugins') '插件变量未恢复。'
    }
    Write-Host '通过：标准 ZIP 目录、SHA-256、任意工作目录及成功/失败后的环境恢复。'
} finally {
    Set-Location $originalLocation
    foreach ($key in $keys) { [Environment]::SetEnvironmentVariable($key, $environment[$key], 'Process') }
    Remove-Variable -Name ZZAPK_TEST_EXIT_CODE -Scope Global -ErrorAction SilentlyContinue
    Remove-Item -LiteralPath $root -Recurse -Force
}
