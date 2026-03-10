@echo off
set SRC=C:\Users\Milosh Xhavid\get-shit-done\build\ChordJoystick_artefacts\Release\VST3\Chord Joystick Controller (BETA).vst3
set DST=C:\Users\Milosh Xhavid\AppData\Local\Programs\Common\VST3\Chord Joystick Controller (BETA).vst3
mkdir "%DST%" 2>nul
xcopy /E /I /Y "%SRC%" "%DST%\" > C:\Users\Milosh^ Xhavid\get-shit-done\copy-vst-out.txt 2>&1
echo ExitCode=%ERRORLEVEL% >> C:\Users\Milosh^ Xhavid\get-shit-done\copy-vst-out.txt
