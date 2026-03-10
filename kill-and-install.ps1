Stop-Process -Name "Ableton Live 11 Suite" -Force -ErrorAction SilentlyContinue
Stop-Process -Name "Ableton Index" -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 1

$src = "C:\Users\Milosh Xhavid\get-shit-done\build\ChordJoystick_artefacts\Release\VST3\Chord Joystick Controller (BETA).vst3"
$dst = "C:\Program Files\Common Files\VST3\Chord Joystick Controller (BETA).vst3"

Remove-Item -Recurse -Force $dst -ErrorAction SilentlyContinue
Copy-Item -Path $src -Destination "C:\Program Files\Common Files\VST3\" -Recurse

$dll = Get-Item "$dst\Contents\x86_64-win\Chord Joystick Controller (BETA).vst3"
Write-Host "Installed: $($dll.LastWriteTime)  $($dll.Length) bytes"
