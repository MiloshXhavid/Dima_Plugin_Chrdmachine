$root = "C:\Users\Milosh Xhavid\get-shit-done"
$outFile = "C:\Users\Milosh Xhavid\Desktop\Dima_Plug-in\ChordJoystickMK2-FullContext.txt"

function AddSection {
    param([string]$label, [string]$path)
    "" | Out-File $outFile -Append -Encoding utf8
    ("=" * 80) | Out-File $outFile -Append -Encoding utf8
    $label | Out-File $outFile -Append -Encoding utf8
    ("=" * 80) | Out-File $outFile -Append -Encoding utf8
    "" | Out-File $outFile -Append -Encoding utf8
    Get-Content $path -Raw | Out-File $outFile -Append -Encoding utf8
}

# Header
"# ChordJoystick MK2 - Complete Project Context" | Out-File $outFile -Encoding utf8
"Generated: $(Get-Date -Format 'yyyy-MM-dd HH:mm')" | Out-File $outFile -Append -Encoding utf8
"" | Out-File $outFile -Append -Encoding utf8
"Upload this file to Claude Desktop to get full context on the project." | Out-File $outFile -Append -Encoding utf8
"Re-run build-context.ps1 from the get-shit-done repo to regenerate after changes." | Out-File $outFile -Append -Encoding utf8

# ── 1. Project docs ──
AddSection "FILE: README.md" "$root\README.md"
AddSection "FILE: .continue-here.md" "$root\.continue-here.md"
AddSection "FILE: CMakeLists.txt" "$root\CMakeLists.txt"
AddSection "FILE: installer/DimaChordJoystick-Setup.iss" "$root\installer\DimaChordJoystick-Setup.iss"

# ── 2. Source headers ──
Get-ChildItem "$root\Source\*.h" | ForEach-Object {
    AddSection "FILE: Source/$($_.Name)" $_.FullName
}

# ── 3. Source implementations ──
Get-ChildItem "$root\Source\*.cpp" | ForEach-Object {
    AddSection "FILE: Source/$($_.Name)" $_.FullName
}

# ── 4. Tests ──
Get-ChildItem "$root\Tests\*.cpp" | ForEach-Object {
    AddSection "FILE: Tests/$($_.Name)" $_.FullName
}

# ── 5. Planning docs (architecture, phases, roadmap, decisions) ──
Get-ChildItem "$root\.planning" -Recurse -File | ForEach-Object {
    $rel = $_.FullName.Substring($root.Length + 1).Replace('\', '/')
    AddSection "FILE: $rel" $_.FullName
}

# ── 6. Claude config ──
if (Test-Path "$root\.claude") {
    Get-ChildItem "$root\.claude" -Recurse -File | ForEach-Object {
        $rel = $_.FullName.Substring($root.Length + 1).Replace('\', '/')
        AddSection "FILE: $rel" $_.FullName
    }
}

# ── 7. Build/install scripts ──
$rootExtras = @('.gitignore', 'build-release.bat', 'copy-vst.bat', 'cj-install.ps1', 'fix-install.ps1')
foreach ($f in $rootExtras) {
    $p = "$root\$f"
    if (Test-Path $p) { AddSection "FILE: $f" $p }
}

# Report
$size = [math]::Round((Get-Item $outFile).Length / 1024)
Write-Host "Done: $outFile ($size KB)"
