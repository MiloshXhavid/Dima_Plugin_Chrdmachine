$src   = 'C:\Users\Milosh Xhavid\get-shit-done\build\ChordJoystick_artefacts\Release\VST3\Arcade Chord Control (BETA-Test).vst3'
$local = 'C:\Users\Milosh Xhavid\AppData\Local\Programs\Common\VST3'
$old   = Join-Path $local 'Arcade Chord Control (BETA-Test).vst3'
if (Test-Path $old) { Remove-Item $old -Recurse -Force }
Copy-Item -LiteralPath $src -Destination $local -Recurse -Force
Write-Host "Done. Local VST3 updated."
