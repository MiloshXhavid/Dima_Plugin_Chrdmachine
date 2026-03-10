Write-Host "`nSYSTEM VST3:"
Get-ChildItem 'C:\Program Files\Common Files\VST3\' -ErrorAction SilentlyContinue | Select-Object Name, LastWriteTime | Format-Table -AutoSize

Write-Host "LOCAL VST3:"
Get-ChildItem 'C:\Users\Milosh Xhavid\AppData\Local\Programs\Common\VST3\' -ErrorAction SilentlyContinue | Select-Object Name, LastWriteTime | Format-Table -AutoSize

Write-Host "REAPER CACHE - plugin entries:"
$cache = 'C:\Users\Milosh Xhavid\AppData\Roaming\REAPER\reaper-vstplugins64.ini'
if (Test-Path $cache) {
    Select-String -Path $cache -Pattern 'Joystick|Chord|DIMEA|joystick' | ForEach-Object { $_.Line }
} else {
    Write-Host "(cache file not found)"
}
