@echo off
set SRC=C:\Users\Milosh Xhavid\get-shit-done\build\ChordJoystick_artefacts\Release\VST3\Chord Joystick Controller (BETA).vst3
set DST=C:\Users\Milosh Xhavid\AppData\Local\Programs\Common\VST3\Chord Joystick Controller (BETA).vst3
echo Copying warp-tunnel build to local VST3...
xcopy /E /I /Y "%SRC%" "%DST%\"
echo Done. ExitCode=%ERRORLEVEL%
pause
