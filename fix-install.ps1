# Run this as Administrator to fix the ChordJoystick VST3 install
# Right-click fix-install.ps1 → "Run with PowerShell" (or open admin PowerShell and run it)

$src  = "C:\Users\Milosh Xhavid\get-shit-done\build\ChordJoystick_artefacts\Release\VST3\Joystick Chord Control (β-test).vst3"
$dst  = "C:\Program Files\Common Files\VST3\Joystick Chord Control (β-test).vst3"

Write-Host "Removing old bundle..."
Remove-Item -Recurse -Force $dst

Write-Host "Copying new bundle..."
Copy-Item -Path $src -Destination "C:\Program Files\Common Files\VST3\" -Recurse

Write-Host ""
Write-Host "Done. Installed DLL:"
Get-ChildItem -Recurse $dst | Select-Object FullName, LastWriteTime, Length

