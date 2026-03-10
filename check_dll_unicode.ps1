# Check both ASCII and Unicode (UTF-16LE) strings in the DLL
$path = 'C:\Program Files\Common Files\VST3\DIMEA CHORD JOYSTICK MK2.vst3\Contents\x86_64-win\DIMEA CHORD JOYSTICK MK2.vst3'
$bytes = [System.IO.File]::ReadAllBytes($path)

# ASCII search
$ascii = [System.Text.Encoding]::ASCII.GetString($bytes)
$asciiHits = [regex]::Matches($ascii, '(?i)(yourcompany|your.company)')
Write-Host "ASCII hits for 'yourcompany': $($asciiHits.Count)"

# UTF-16LE search
$unicode = [System.Text.Encoding]::Unicode.GetString($bytes)
$uHits = [regex]::Matches($unicode, '(?i)(yourcompany|your.company)')
Write-Host "Unicode hits for 'yourcompany': $($uHits.Count)"
foreach ($h in $uHits) { Write-Host "  -> '$($h.Value)'" }

# Also check nested AppData DLL
$path2 = 'C:\Users\Milosh Xhavid\AppData\Local\Programs\Common\VST3\DIMEA CHORD JOYSTICK MK2.vst3\DIMEA CHORD JOYSTICK MK2.vst3\Contents\x86_64-win\DIMEA CHORD JOYSTICK MK2.vst3'
if (Test-Path $path2) {
    $bytes2 = [System.IO.File]::ReadAllBytes($path2)
    $ascii2 = [System.Text.Encoding]::ASCII.GetString($bytes2)
    $hits2 = [regex]::Matches($ascii2, '(?i)(yourcompany)')
    Write-Host "Nested AppData DLL - ASCII 'yourcompany' hits: $($hits2.Count)"
    $info = Get-Item $path2 | Select-Object LastWriteTime, Length
    Write-Host "Nested AppData DLL timestamp: $($info.LastWriteTime), size: $($info.Length)"
} else {
    Write-Host "Nested AppData DLL not found at expected path"
}
