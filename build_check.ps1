Set-Location 'C:\Users\Milosh Xhavid\get-shit-done'
$r = cmake --build build --config Release --target ChordJoystick_VST3 2>&1
$r | Out-File 'C:\Users\Milosh Xhavid\get-shit-done\build_check_out.txt'
