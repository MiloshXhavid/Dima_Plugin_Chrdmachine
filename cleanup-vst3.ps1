$paths = @(
    "C:\Program Files\Common Files\VST3",
    "$env:LOCALAPPDATA\Programs\Common\VST3"
)

$keep = "Chord Joystick Controller (BETA).vst3"

foreach ($dir in $paths) {
    Write-Host "`n=== $dir ==="
    if (-not (Test-Path $dir)) { Write-Host "  (not found)"; continue }

    $items = Get-ChildItem $dir -Filter "*.vst3"
    foreach ($item in $items) {
        if ($item.Name -eq $keep) {
            $dll = "$($item.FullName)\Contents\x86_64-win\$keep"
            $ts = if (Test-Path $dll) { (Get-Item $dll).LastWriteTime } else { "DLL not found" }
            Write-Host "  [KEEP] $($item.Name)  ($ts)"
        } else {
            Write-Host "  [DELETE] $($item.Name)"
            Remove-Item -Recurse -Force $item.FullName
        }
    }
}

Write-Host "`nDone."
