$systemVST3 = 'C:\Program Files\Common Files\VST3'
$localVST3  = 'C:\Users\Milosh Xhavid\AppData\Local\Programs\Common\VST3'

$toRemove = @(
    'Dimea Arcade - Chord Joystick Controller.vst3',
    'Chord Joystick Control (BETA).vst3',
    'DIMEA CHORD JOYSTICK MK2.vst3',
    'ChordJoystick.vst3',
    'Joystick Chord Control (b-test).vst3'
)

foreach ($dir in @($systemVST3, $localVST3)) {
    if (-not (Test-Path $dir)) { continue }
    foreach ($name in $toRemove) {
        $path = Join-Path $dir $name
        if (Test-Path -LiteralPath $path) {
            cmd.exe /c "rd /s /q `"$path`"" 2>$null
            Write-Host "Removed: $path"
        }
    }
    foreach ($g in @('Chord Joystick*.vst3','Joystick*.vst3','DIMEA*.vst3','ChordJoystick*.vst3')) {
        Get-ChildItem -Path $dir -Filter $g -ErrorAction SilentlyContinue | Where-Object { $_.Name -ne 'Chord Joystick Controller (BETA).vst3' } | ForEach-Object {
            cmd.exe /c "rd /s /q `"$($_.FullName)`"" 2>$null
            Write-Host "Removed (glob): $($_.FullName)"
        }
    }
}

# Copy new build to local VST3 as well
$src = 'C:\Users\Milosh Xhavid\get-shit-done\build\ChordJoystick_artefacts\Release\VST3\Chord Joystick Controller (BETA).vst3'
$dst = 'C:\Users\Milosh Xhavid\AppData\Local\Programs\Common\VST3\Chord Joystick Controller (BETA).vst3'
if (Test-Path -LiteralPath $src) {
    Copy-Item -LiteralPath $src -Destination $localVST3 -Recurse -Force
    Write-Host "Installed to local: $dst"
}

Write-Host ""
Write-Host "=== Installed VST3 plugins ==="
Write-Host "System:"
Get-ChildItem $systemVST3 -Filter '*.vst3' -ErrorAction SilentlyContinue | ForEach-Object { Write-Host "  $($_.Name)" }
Write-Host "Local:"
Get-ChildItem $localVST3 -Filter '*.vst3' -ErrorAction SilentlyContinue | ForEach-Object { Write-Host "  $($_.Name)" }
