$name = "Arcade Chord Control (BETA-Test)"
$src = "C:\Users\Milosh Xhavid\get-shit-done\build\ChordJoystick_artefacts\Release\VST3\$name.vst3"
$dst = "$env:LOCALAPPDATA\Programs\Common\VST3\$name.vst3"

Remove-Item -Recurse -Force $dst -ErrorAction SilentlyContinue
Copy-Item -Path $src -Destination "$env:LOCALAPPDATA\Programs\Common\VST3\" -Recurse

$dll = Get-Item "$dst\Contents\x86_64-win\$name.vst3"
Write-Host "LocalAppData installed: $($dll.LastWriteTime)  $($dll.Length) bytes"
