$path = 'C:\Program Files\Common Files\VST3\DIMEA CHORD JOYSTICK MK2.vst3\Contents\x86_64-win\DIMEA CHORD JOYSTICK MK2.vst3'
$bytes = [System.IO.File]::ReadAllBytes($path)
$unicode = [System.Text.Encoding]::Unicode.GetString($bytes)

$match = [regex]::Match($unicode, '(?i)yourcompany')
if ($match.Success) {
    $start = [Math]::Max(0, $match.Index - 200)
    $len   = [Math]::Min(400, $unicode.Length - $start)
    $context = $unicode.Substring($start, $len)
    # Remove non-printable chars for display
    $printable = [regex]::Replace($context, '[^\x20-\x7E]', '.')
    Write-Host "Context around Unicode 'yourcompany' (offset $($match.Index)):"
    Write-Host $printable
}
