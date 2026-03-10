$dir = "C:\Users\Milosh Xhavid\AppData\Local\Programs\Common\VST3"
Get-ChildItem $dir | ForEach-Object {
    if ($_.Name -match "Joystick") {
        Write-Host "Deleting: $($_.FullName)"
        Remove-Item $_.FullName -Recurse -Force
    }
}
Write-Host "Done"
