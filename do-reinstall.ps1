# do-reinstall.ps1 — clean all old VST3 copies and install fresh build
$localVST3  = 'C:\Users\Milosh Xhavid\AppData\Local\Programs\Common\VST3'
$systemVST3 = 'C:\Program Files\Common Files\VST3'
$buildSrc   = 'C:\Users\Milosh Xhavid\get-shit-done\build\ChordJoystick_artefacts\Release\VST3\Arcade Chord Control (BETA-Test).vst3'
$reaperCache = 'C:\Users\Milosh Xhavid\AppData\Roaming\REAPER\reaper-vstplugins64.ini'

$exact = @(
    'DIMEA CHORD JOYSTICK MK2.vst3',
    'Chord Joystick Controller (BETA).vst3',
    'Chord Joystick Control (BETA).vst3',
    'ChordJoystick.vst3',
    'ChordJoystick MK2.vst3',
    'Dimea Arcade - Chord Joystick Controller.vst3',
    'Joystick Chord Control (b-test).vst3',
    'Joystick Chord Control (β-test).vst3',
    'Dima_Chord_Joy_MK2.vst3'
)
$globs = @('Chord Joystick*.vst3','Joystick*.vst3','DIMEA*.vst3','ChordJoystick*.vst3','Dima*.vst3')

function Remove-VST3 ($path) {
    if (Test-Path -LiteralPath $path) {
        & cmd.exe /c "rd /s /q `"$path`"" 2>&1 | Out-Null
        if (Test-Path -LiteralPath $path) {
            Write-Host "  FAILED (locked): $path" -ForegroundColor Red
        } else {
            Write-Host "  Deleted: $(Split-Path $path -Leaf)"
        }
    }
}

# 1. Kill Reaper
Write-Host "`n[1] Stopping Reaper..." -ForegroundColor Cyan
$rp = Get-Process -Name 'reaper' -ErrorAction SilentlyContinue
if ($rp) { $rp | Stop-Process -Force; Start-Sleep -Seconds 2; Write-Host "    Stopped." }
else { Write-Host "    Not running." }

# 2. Remove from local (no admin needed)
Write-Host "`n[2] Removing from local VST3..." -ForegroundColor Cyan
foreach ($name in $exact) { Remove-VST3 (Join-Path $localVST3 $name) }
foreach ($g in $globs) {
    Get-ChildItem -Path $localVST3 -Filter $g -ErrorAction SilentlyContinue | ForEach-Object { Remove-VST3 $_.FullName }
}

# 3. Remove from system (needs admin)
Write-Host "`n[3] Removing from system VST3..." -ForegroundColor Cyan
foreach ($name in $exact) { Remove-VST3 (Join-Path $systemVST3 $name) }
foreach ($g in $globs) {
    Get-ChildItem -Path $systemVST3 -Filter $g -ErrorAction SilentlyContinue | ForEach-Object { Remove-VST3 $_.FullName }
}

# 4. Clear Reaper cache
Write-Host "`n[4] Clearing Reaper cache..." -ForegroundColor Cyan
if (Test-Path -LiteralPath $reaperCache) {
    Remove-Item -LiteralPath $reaperCache -Force
    Write-Host "    Cache deleted."
} else {
    Write-Host "    Cache not found (already clean)."
}

# 5. Install fresh build to both paths
Write-Host "`n[5] Installing fresh build..." -ForegroundColor Cyan
if (-not (Test-Path -LiteralPath $buildSrc)) {
    Write-Host "    ERROR: Build not found at $buildSrc" -ForegroundColor Red
    exit 1
}
Copy-Item -LiteralPath $buildSrc -Destination $localVST3 -Recurse -Force
Write-Host "    Installed to local: $localVST3"

Copy-Item -LiteralPath $buildSrc -Destination $systemVST3 -Recurse -Force
Write-Host "    Installed to system: $systemVST3"

# 6. Summary
Write-Host "`n[6] Installed VST3 plugins:" -ForegroundColor Cyan
Write-Host "  System:"
Get-ChildItem $systemVST3 -Filter '*.vst3' -ErrorAction SilentlyContinue | ForEach-Object { Write-Host "    $($_.Name)" }
Write-Host "  Local:"
Get-ChildItem $localVST3 -Filter '*.vst3' -ErrorAction SilentlyContinue | ForEach-Object { Write-Host "    $($_.Name)" }

Write-Host "`nDone. Open your DAW — it will do a clean rescan on startup.`n" -ForegroundColor Green
