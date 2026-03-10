Get-Process | Where-Object { $_.Name -match 'msbuild|link|devenv|vs|ableton|live|reaper|pluginval' } | Select-Object Name, Id
