Set-Location 'C:\Users\Milosh Xhavid\get-shit-done\build'
$out = & cmake --build . --config Release --target ChordJoystick_VST3 2>&1
$out | Out-File 'C:\Users\Milosh Xhavid\get-shit-done\build31_out.txt'
$out | Select-String -Pattern 'error|warning|succeeded|FAILED|Build FAILED' | Select-Object -Last 20
