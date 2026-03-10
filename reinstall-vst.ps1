# reinstall-vst.ps1 — Nuclear VST3 reinstall for Reaper
# Run as Administrator (right-click → Run with PowerShell, or from admin shell)
# Kills Reaper, wipes ALL known install locations + cache, then fresh-installs.

$ErrorActionPreference = "Stop"

$buildSrc   = "C:\Users\Milosh Xhavid\get-shit-done\build\ChordJoystick_artefacts\Release\VST3\Chord Joystick Controller (BETA).vst3"
$systemVST3 = "C:\Program Files\Common Files\VST3"
$localVST3  = "C:\Users\Milosh Xhavid\AppData\Local\Programs\Common\VST3"
$reaperCache = "C:\Users\Milosh Xhavid\AppData\Roaming\REAPER\reaper-vstplugins64.ini"

# --- 1. Kill Reaper first (cache must be wiped while it's NOT running) --------
Write-Host "`n[1] Stopping Reaper..." -ForegroundColor Cyan
$reaperProcs = Get-Process -Name "reaper" -ErrorAction SilentlyContinue
if ($reaperProcs) {
    $reaperProcs | Stop-Process -Force
    Start-Sleep -Seconds 2
    Write-Host "    Reaper stopped."
} else {
    Write-Host "    Reaper not running."
}

# --- 2. Wipe every known VST3 copy (old + new names) -------------------------
Write-Host "`n[2] Removing all plugin copies..." -ForegroundColor Cyan

$exact = @(
    "Chord Joystick Controller (BETA).vst3",
    "Chord Joystick Control (BETA).vst3",
    "Dimea Arcade - Chord Joystick Controller.vst3",
    "DIMEA CHORD JOYSTICK MK2.vst3",
    "ChordJoystick.vst3",
    "Joystick Chord Control (β-test).vst3",
    "Joystick Chord Control (b-test).vst3"
)
$globs = @("Chord Joystick*.vst3", "Joystick*.vst3", "DIMEA*.vst3", "ChordJoystick*.vst3")

function Remove-VST3Bundle ($path) {
    if (Test-Path -LiteralPath $path) {
        & cmd.exe /c "rd /s /q `"$path`"" 2>&1 | Out-Null
        if (Test-Path -LiteralPath $path) {
            Write-Host "    FAILED (still locked): $path" -ForegroundColor Red
        } else {
            Write-Host "    Deleted: $path"
        }
    }
}

foreach ($dir in @($systemVST3, $localVST3)) {
    if (-not (Test-Path $dir)) { continue }
    foreach ($name in $exact) { Remove-VST3Bundle (Join-Path $dir $name) }
    foreach ($g in $globs) {
        Get-ChildItem -Path $dir -Filter $g -ErrorAction SilentlyContinue | ForEach-Object {
            Remove-VST3Bundle $_.FullName
        }
    }
}

# --- 3. Wipe Reaper VST cache -------------------------------------------------
Write-Host "`n[3] Clearing Reaper VST cache..." -ForegroundColor Cyan
if (Test-Path -LiteralPath $reaperCache) {
    Remove-Item -LiteralPath $reaperCache -Force
    Write-Host "    Deleted $reaperCache"
} else {
    Write-Host "    Cache file not found (already clean)."
}

# --- 4. Verify build exists ---------------------------------------------------
Write-Host "`n[4] Checking build source..." -ForegroundColor Cyan
if (-not (Test-Path -LiteralPath $buildSrc)) {
    Write-Host "    ERROR: Build not found at:`n    $buildSrc" -ForegroundColor Red
    Write-Host "    Build the plugin first, then re-run this script."
    exit 1
}
Write-Host "    Found: $buildSrc"

# --- 5. Install to system VST3 -----------------------------------------------
Write-Host "`n[5] Installing to system VST3..." -ForegroundColor Cyan
$dst = Join-Path $systemVST3 "Chord Joystick Controller (BETA).vst3"
Copy-Item -LiteralPath $buildSrc -Destination $systemVST3 -Recurse -Force
Write-Host "    Installed to: $dst"

# --- 6. Verify install --------------------------------------------------------
Write-Host "`n[6] Installed files:" -ForegroundColor Cyan
Get-ChildItem -Recurse -LiteralPath $dst | Select-Object FullName, LastWriteTime, @{N='KB';E={[math]::Round($_.Length/1KB,1)}} | Format-Table -AutoSize

Write-Host "`nDone. Open Reaper — it will do a clean full rescan on startup.`n" -ForegroundColor Green
