$content = Get-Content 'C:\Users\Milosh Xhavid\get-shit-done\build\ChordJoystick.vcxproj' -Raw
$match = [regex]::Match($content, 'JucePlugin_Manufacturer=\\?"([^\\;"]+)')
if ($match.Success) {
    Write-Host "JucePlugin_Manufacturer = $($match.Groups[1].Value)"
} else {
    Write-Host "Not found - searching for COMPANY..."
    $m2 = [regex]::Match($content, 'COMPANY[^;]*')
    Write-Host $m2.Value
}
