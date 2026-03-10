$target = "C:\Program Files\Common Files\VST3\Chord Joystick Controller (BETA).vst3\Contents\x86_64-win\Chord Joystick Controller (BETA).vst3"
$procs = Get-Process | Where-Object { try { $_.Modules | Where-Object { $_.FileName -eq $target } } catch { $false } }
if ($procs) { $procs | Select-Object Name, Id } else { Write-Host "No process found via modules. Try closing Ableton/Reaper/FL." }
