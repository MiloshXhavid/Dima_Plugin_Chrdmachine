$src  = 'C:\Users\Milosh Xhavid\get-shit-done\build\ChordJoystick_artefacts\Release\VST3\Arcade Chord Control (BETA-Test).vst3\Contents\x86_64-win\Arcade Chord Control (BETA-Test).vst3'
$dst1 = 'C:\Program Files\Common Files\VST3\Arcade Chord Control (BETA-Test).vst3\Contents\x86_64-win\Arcade Chord Control (BETA-Test).vst3'
$dst2 = 'C:\Users\Milosh Xhavid\AppData\Local\Programs\Common\VST3\Arcade Chord Control (BETA-Test).vst3\Contents\x86_64-win\Arcade Chord Control (BETA-Test).vst3'
Copy-Item -Path $src -Destination $dst1 -Force
Copy-Item -Path $src -Destination $dst2 -Force
Get-Item $dst1, $dst2 | Select-Object LastWriteTime, Length | Format-List
