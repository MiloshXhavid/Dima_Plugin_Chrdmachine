Set-Location 'C:\Users\Milosh Xhavid\get-shit-done\build'
$out = & cmake --build . --config Release --target ChordJoystick_VST3 2>&1
$out | Out-File 'C:\Users\Milosh Xhavid\get-shit-done\build32_out.txt'
$errors = $out | Select-String -Pattern ' error C| error LNK|Build FAILED'
if ($errors) { $errors } else { Write-Host "Build OK" }
