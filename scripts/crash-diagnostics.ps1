# Crash report check (#381) for Windows CI.
#
# Usage: scripts\crash-diagnostics.ps1 -App <path\to\MaximumTrainer.exe>
#
# 1. Launches the app with MT_CRASH_TEST=1, which crashes on purpose during the
#    QtWebEngine init, and checks the report: fault line, symbolized stack,
#    module list, GPU line and the minidump.
# 2. Launches it again normally and checks the recovery: safe graphics turned
#    on and Chromium's own log captured.
param(
    [Parameter(Mandatory = $true)][string]$App
)
$ErrorActionPreference = 'Stop'

$logDir = Join-Path $env:APPDATA 'MaximumTrainer\MaximumTrainer_Redux'
$log = Join-Path $logDir 'MaximumTrainer.log'
$dump = Join-Path $logDir 'MaximumTrainer-crash.dmp'
$chromiumLog = Join-Path $logDir 'MaximumTrainer-webengine.log'

function Fail([string]$message) {
    Write-Host "::error::crash diagnostics: $message"
    exit 1
}

function Run-App([hashtable]$environment) {
    $startLine = 0
    if (Test-Path $log) { $startLine = @(Get-Content -LiteralPath $log -Encoding UTF8).Count }
    foreach ($key in $environment.Keys) { Set-Item -Path "env:$key" -Value $environment[$key] }
    $proc = Start-Process -FilePath $App -ArgumentList '--startup-check' -PassThru
    $null = $proc.Handle
    if (-not $proc.WaitForExit(90000)) { $proc.Kill(); Fail 'app did not exit within 90 s' }
    foreach ($key in $environment.Keys) { Remove-Item -Path "env:$key" }
    $runLog = @(Get-Content -LiteralPath $log -Encoding UTF8 | Select-Object -Skip $startLine)
    Write-Host "=== log for this run (exit code $($proc.ExitCode)) ==="
    $runLog | ForEach-Object { Write-Host $_ }
    Write-Host '==============================='
    return @{ ExitCode = $proc.ExitCode; Log = $runLog }
}

function Require($runLog, [string]$pattern, [string]$what) {
    if (-not ($runLog | Where-Object { $_ -match $pattern })) { Fail "missing: $what" }
}

# No Windows Error Reporting dialog: it would keep the crashed process alive.
New-Item -Path 'HKCU:\Software\Microsoft\Windows\Windows Error Reporting' -Force | Out-Null
Set-ItemProperty -Path 'HKCU:\Software\Microsoft\Windows\Windows Error Reporting' -Name DontShowUI -Value 1
Remove-Item -LiteralPath $dump, $chromiumLog -ErrorAction SilentlyContinue

$crash = Run-App @{ MT_NO_NETWORK = '1'; MT_CRASH_TEST = '1' }
if ($crash.ExitCode -eq 0) { Fail 'the crash test exited normally' }
Require $crash.Log '\] GPU 0: ' 'GPU adapter line'
Require $crash.Log 'fatal: unhandled exception 0xc0000005 during GlobalVars: QWebEngineProfile init' 'fatal line'
Require $crash.Log 'fault at MaximumTrainer\.exe\+0x[0-9a-f]+ on main thread .*\(write of address 0x0\)' 'fault line'
Require $crash.Log 'stack \([1-9][0-9]* frames\):' 'stack header'
Require $crash.Log '#0 MaximumTrainer\.exe\+0x[0-9a-f]+ StartupTrace::crashForTesting\+0x[0-9a-f]+ \(startup_trace\.cpp:[0-9]+\)' 'symbolized crash frame (PDB)'
Require $crash.Log 'GlobalVars::GlobalVars' 'caller frame'
Require $crash.Log '(driver|third-party) module: |no driver or third-party modules loaded' 'module list'
Require $crash.Log 'crash dump written to ' 'dump line'
if (-not (Test-Path $dump)) { Fail "no dump at $dump" }
Write-Host ("minidump: {0:N0} KB" -f ((Get-Item $dump).Length / 1KB))

$recovery = Run-App @{ MT_NO_NETWORK = '1' }
if ($recovery.ExitCode -ne 0) { Fail "the recovery launch exited with code $($recovery.ExitCode)" }
Require $recovery.Log 'WebEngine safe graphics: on \(previous launch crashed' 'safe graphics after the crash'
Require $recovery.Log 'WebEngine log: ' 'Chromium log line'
if (-not (Test-Path $chromiumLog) -or (Get-Item $chromiumLog).Length -eq 0) { Fail "Chromium log empty or missing: $chromiumLog" }
Write-Host '=== Chromium log ==='
Get-Content -LiteralPath $chromiumLog | Select-Object -First 40 | ForEach-Object { Write-Host $_ }

# Leave the profile as the other steps expect it: GPU on, no remembered crash.
Remove-ItemProperty -Path 'HKCU:\Software\MaximumTrainer\MaximumTrainer_Redux\startup' -Name webengine_safe_mode -ErrorAction SilentlyContinue
Write-Host 'crash diagnostics: OK'
