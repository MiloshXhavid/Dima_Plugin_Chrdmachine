$bytes = [System.IO.File]::ReadAllBytes('C:\Program Files\Common Files\VST3\DIMEA CHORD JOYSTICK MK2.vst3\Contents\x86_64-win\DIMEA CHORD JOYSTICK MK2.vst3')
$text = [System.Text.Encoding]::ASCII.GetString($bytes)
$hits = [regex]::Matches($text, '(?i)(yourcompany|DIMEA)')
$unique = $hits | ForEach-Object { $_.Value } | Sort-Object -Unique
$unique | ForEach-Object { Write-Host $_ }
