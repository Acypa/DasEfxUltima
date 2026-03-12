[Setup]
AppName=Das EFX 1993 Studio
AppVersion=1.0
DefaultDirName={commoncf}\VST3
DisableDirPage=yes
OutputBaseFilename=DasEfxUltimaInstaller
Compression=lzma
SolidCompression=yes

[Files]
Source: "build\DasEfxUltima_artefacts\Release\VST3\DasEfxUltima.vst3"; DestDir: "{commoncf}\VST3"; Flags: ignoreversion

[Icons]
Name: "{group}\Das EFX 1993 Studio"; Filename: "{app}\DasEfxUltima.vst3"