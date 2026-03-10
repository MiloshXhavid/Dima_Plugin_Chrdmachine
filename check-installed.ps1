$name = "Chord Joystick Controller (BETA).vst3"
$buildDll = "C:\Users\Milosh Xhavid\get-shit-done\build\ChordJoystick_artefacts\Release\VST3\$name\Contents\x86_64-win\$name"
$buildTs = (Get-Item $buildDll).LastWriteTime

Write-Host "Build:          $buildTs"

$paths = @(
    "C:\Program Files\Common Files\VST3\$name\Contents\x86_64-win\$name",
    "$env:LOCALAPPDATA\Programs\Common\VST3\$name\Contents\x86_64-win\$name"
)

foreach ($p in $paths) {
    if (Test-Path $p) {
        $ts = (Get-Item $p).LastWriteTime
        $match = if ($ts -eq $buildTs) { "OK" } else { "OUTDATED" }
        Write-Host "[$match] $p`n        $ts"
    } else {
        Write-Host "[MISSING] $p"
    }
}
