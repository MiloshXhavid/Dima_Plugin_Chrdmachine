cmake --build 'C:\Users\Milosh Xhavid\get-shit-done\build' --config Release 2>&1 | Where-Object { $_ -match 'error|->|Installing|warning C' }
