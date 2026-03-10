$content = Get-Content 'C:\Users\Milosh Xhavid\get-shit-done\build\ChordJoystick_VST3.vcxproj' -Raw
$match = [regex]::Match($content, 'JucePlugin_Manufacturer=\\?"([^\\;"]+)')
if ($match.Success) {
    Write-Host "VST3 target JucePlugin_Manufacturer = $($match.Groups[1].Value)"
} else {
    Write-Host "JucePlugin_Manufacturer not found in VST3 vcxproj"
}

# Also check for JUCE_COMPANY_NAME property directly
$match2 = [regex]::Match($content, 'JUCE_COMPANY_NAME=([^;]+)')
if ($match2.Success) {
    Write-Host "JUCE_COMPANY_NAME = $($match2.Groups[1].Value)"
}

# Show any yourcompany hits
$hits = [regex]::Matches($content, 'yourcompany')
Write-Host "yourcompany occurrences in VST3 vcxproj: $($hits.Count)"
