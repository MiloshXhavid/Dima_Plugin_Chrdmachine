$name = "Arcade Chord Control (BETA-Test)"
$src = "C:\Users\Milosh Xhavid\get-shit-done\build\ChordJoystick_artefacts\Release\VST3\$name.vst3\Contents\x86_64-win\$name.vst3"
$dst = "C:\Program Files\Common Files\VST3\$name.vst3\Contents\x86_64-win\$name.vst3"
Copy-Item -Path $src -Destination $dst -Force
Write-Host "Done. Size: $((Get-Item $dst).Length) bytes, Modified: $((Get-Item $dst).LastWriteTime)"
