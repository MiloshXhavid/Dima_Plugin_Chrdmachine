; DimaChordJoystick-Setup.iss
; Inno Setup 6.7.1 installer for ChordJoystick MK2 VST3 v2.0.0
; Place this file in the installer/ subdirectory of the project root.

[Setup]
AppName=ChordJoystick MK2
AppVersion=1.2
AppPublisher=Dima Xhavid
AppPublisherURL=https://gumroad.com
OutputBaseFilename=DimaChordJoystickMK2-Setup
OutputDir=Output
DefaultDirName={commoncf64}\VST3\ChordJoystick MK2.vst3
LicenseFile=..\LICENSE.txt
PrivilegesRequired=admin
PrivilegesRequiredOverridesAllowed=dialog
DisableDirPage=yes
Compression=lzma
SolidCompression=yes
UninstallDisplayName=ChordJoystick MK2 VST3

[Files]
; Copies the entire .vst3 bundle (directory tree) to the standard VST3 system path.
; Source ends in \* so Inno Setup recurses into the directory.
; DestDir creates ChordJoystick.vst3\ at the destination.
Source: "..\build\ChordJoystick_artefacts\Release\VST3\ChordJoystick MK2.vst3\*"; \
    DestDir: "{commoncf64}\VST3\ChordJoystick MK2.vst3"; \
    Flags: ignoreversion recursesubdirs createallsubdirs

[UninstallDelete]
; Ensure the whole .vst3 bundle directory is removed on uninstall.
Type: filesandordirs; Name: "{commoncf64}\VST3\ChordJoystick MK2.vst3"

; -- Code-signing placeholder --------------------------------------------------
; Uncomment and fill in when a certificate is acquired:
; [Code]
; procedure CurStepChanged(CurStep: TSetupStep);
; var ResultCode: Integer;
; begin
;   if CurStep = ssPostInstall then begin
;     Exec('signtool.exe',
;       'sign /tr http://timestamp.digicert.com /td sha256 /fd sha256 /a ' +
;       '"' + ExpandConstant('{commoncf64}\VST3\ChordJoystick.vst3\Contents\x86_64-win\ChordJoystick.vst3"'),
;       '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
;   end;
; end;
