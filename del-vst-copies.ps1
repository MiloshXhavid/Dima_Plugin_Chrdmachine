# Kill Reaper so it releases the DLL lock
$rp = Get-Process -Name 'reaper' -ErrorAction SilentlyContinue
if ($rp) { $rp | Stop-Process -Force; Start-Sleep -Seconds 2; Write-Host 'Reaper stopped.' }

$systemVST3 = 'C:\Program Files\Common Files\VST3'
$localVST3  = 'C:\Users\Milosh Xhavid\AppData\Local\Programs\Common\VST3'

# All known name patterns (exact + glob)
$exact = @(
    'DIMEA CHORD JOYSTICK MK2.vst3',
    'ChordJoystick.vst3',
    'Joystick Chord Control (β-test).vst3',
    'Joystick Chord Control (b-test).vst3'
)
$globs = @('Joystick*.vst3', 'DIMEA*.vst3', 'ChordJoystick*.vst3')

function Remove-VST3 ($path) {
    if (Test-Path -LiteralPath $path) {
        # cmd rd is more forceful than Remove-Item on locked files
        & cmd.exe /c "rd /s /q `"$path`"" 2>&1 | Out-Null
        if (Test-Path -LiteralPath $path) {
            Write-Host "  FAILED (still locked?): $path" -ForegroundColor Red
        } else {
            Write-Host "  DELETED: $path" -ForegroundColor Yellow
        }
    }
}

foreach ($dir in @($systemVST3, $localVST3)) {
    if (-not (Test-Path $dir)) { continue }
    foreach ($name in $exact) {
        Remove-VST3 (Join-Path $dir $name)
    }
    foreach ($g in $globs) {
        Get-ChildItem -Path $dir -Filter $g -ErrorAction SilentlyContinue | ForEach-Object {
            Remove-VST3 $_.FullName
        }
    }
}

# Clear Reaper cache too
$cache = 'C:\Users\Milosh Xhavid\AppData\Roaming\REAPER\reaper-vstplugins64.ini'
if (Test-Path -LiteralPath $cache) {
    Remove-Item -LiteralPath $cache -Force
    Write-Host "  DELETED cache: $cache" -ForegroundColor Yellow
}

Write-Host "`nAll old copies removed. Now run reinstall-vst.ps1 (as admin) to install fresh." -ForegroundColor Green
