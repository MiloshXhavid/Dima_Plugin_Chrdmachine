; DimaChordJoystick-Setup.iss
; Inno Setup 6.7.1 installer for DIMEA CHORD JOYSTICK MK2 VST3 v1.6
; Place this file in the installer/ subdirectory of the project root.

[Setup]
AppName=DIMEA CHORD JOYSTICK MK2
AppVersion=1.6
AppPublisher=DIMEA
AppPublisherURL=https://www.dimea.com
AppComments=v1.6: Defaults & Bug Fix (octave/scale defaults; stuck-notes fix), Triplet Subdivisions, Random Free Redesign (Poisson density), Looper Perimeter Bar
OutputBaseFilename=DIMEA-ChordJoystickMK2-v1.6-Setup
OutputDir=Output
DefaultDirName={commoncf64}\VST3\DIMEA CHORD JOYSTICK MK2.vst3
PrivilegesRequired=admin
PrivilegesRequiredOverridesAllowed=dialog
DisableDirPage=yes
Compression=lzma
SolidCompression=yes
UninstallDisplayName=DIMEA CHORD JOYSTICK MK2 VST3

[Files]
; Copies the entire .vst3 bundle (directory tree) to the standard VST3 system path.
; Source ends in \* so Inno Setup recurses into the directory.
; DestDir creates DIMEA CHORD JOYSTICK MK2.vst3\ at the destination.
Source: "..\build\ChordJoystick_artefacts\Release\VST3\Chord Joystick Control (BETA).vst3\*"; \
    DestDir: "{commoncf64}\VST3\DIMEA CHORD JOYSTICK MK2.vst3"; \
    Flags: ignoreversion recursesubdirs createallsubdirs

[UninstallDelete]
; Ensure the whole .vst3 bundle directory is removed on uninstall.
Type: filesandordirs; Name: "{commoncf64}\VST3\DIMEA CHORD JOYSTICK MK2.vst3"

[Messages]
WelcomeLabel1=Welcome to DIMEA — congratulations for unlocking musical skills. Let's play some chords! Check out the Website: www.DIMEA.com
WelcomeLabel2=Hi, I am DIMEA. I am currently no certified developer by Microsoft nor MAC OS and this Plugin is currently in Beta-Phase. That's why you will see the warning PopUP by installing this File. Ignore it ;) The Plugin will enroll with all necessary certificates once it's out for the market. How cares about these warnings anyway. Have fun exploring Harmonization and give me your feedback under dimitri.daehler@dimea.com!
