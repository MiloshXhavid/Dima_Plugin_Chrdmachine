Remove-Item -Recurse -Force "C:\Program Files\Common Files\VST3\ChordJoystick.vst3" -ErrorAction SilentlyContinue
Copy-Item -Path "C:\Users\Milosh Xhavid\get-shit-done\build\ChordJoystick_artefacts\Release\VST3\ChordJoystick.vst3" -Destination "C:\Program Files\Common Files\VST3\" -Recurse
Write-Host "Done."
Get-ChildItem -Recurse "C:\Program Files\Common Files\VST3\ChordJoystick.vst3" | Select-Object FullName, LastWriteTime
