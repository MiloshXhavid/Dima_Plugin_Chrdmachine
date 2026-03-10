# Find and search generated JUCE header files
$buildDir = 'C:\Users\Milosh Xhavid\get-shit-done\build'
$results = Get-ChildItem $buildDir -Recurse -Include '*.h','*.cpp','*.txt','*.cmake' -ErrorAction SilentlyContinue |
    Where-Object { $_.FullName -notlike '*_deps*' } |
    Select-String -Pattern 'yourcompany' -ErrorAction SilentlyContinue

foreach ($r in $results) {
    Write-Host "$($r.Filename):$($r.LineNumber) => $($r.Line.Trim())"
}

if (-not $results) {
    Write-Host "No hits outside _deps"
}
