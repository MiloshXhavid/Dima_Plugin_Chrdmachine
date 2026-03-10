$dst = 'C:\Program Files\Common Files\VST3\Joystick Chord Control (β-test).vst3'
$src = 'C:\Users\Milosh Xhavid\get-shit-done\build\ChordJoystick_artefacts\Release\VST3\Joystick Chord Control (β-test).vst3'
& cmd.exe /c "rd /s /q `"$dst`"" 2>&1 | Out-Null
Copy-Item -LiteralPath $src -Destination 'C:\Program Files\Common Files\VST3\' -Recurse -Force
$dll = Get-Item "$dst\Contents\x86_64-win\Joystick Chord Control (β-test).vst3"
Write-Host "Installed: $($dll.LastWriteTime)  $($dll.Length) bytes"
