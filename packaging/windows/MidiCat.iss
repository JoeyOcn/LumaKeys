; Instalador de MidiCat para Windows (Inno Setup 6)
#ifndef AppVersion
  #define AppVersion "1.5.0"
#endif
#define ArtDir "..\..\build\MidiCat_artefacts\Release"

[Setup]
; Mismo AppId que la versión vieja (MidiSky): así la reemplaza en vez de quedar las dos
AppId={{8F3C2A51-6E2B-4D7A-9C1E-5B7A2D9E4F10}
AppName=MidiCat
AppVersion={#AppVersion}
AppVerName=MidiCat {#AppVersion}
AppPublisher=Joey Ocn
AppPublisherURL=https://github.com/JoeyOcn/LumaKeys
DefaultDirName={autopf}\MidiCat
DefaultGroupName=MidiCat
UsePreviousAppDir=no
UsePreviousGroup=no
DisableProgramGroupPage=yes
DisableDirPage=yes
OutputDir=..\..\dist
OutputBaseFilename=MidiCat-Instalador-Windows
SetupIconFile=MidiCat.ico
Compression=lzma2
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
WizardStyle=modern
ShowLanguageDialog=no
UninstallDisplayName=MidiCat
UninstallDisplayIcon={app}\MidiCat.exe
CloseApplications=yes

[Languages]
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"

[Messages]
spanish.WelcomeLabel2=Esto instala MidiCat en tu computadora:%n%n• El plugin VST3 en la carpeta de plugins de Windows, para que Ableton lo encuentre solo.%n• La app MidiCat para usarla sin Ableton.%n%nSi tenías la versión vieja (MidiSky), se quita sola.%nSi Ableton está abierto, ciérralo antes de continuar.
spanish.FinishedLabel=MidiCat quedó instalado.%n%nAbre Ableton y búscalo en Plug-Ins > VST3 > Joey Ocn. Si no aparece: Opciones > Preferencias > Plug-Ins, activa "Usar carpeta del sistema VST3" y dale "Reescanear".

[Tasks]
Name: "desktopicon"; Description: "Crear un acceso directo de la app en el escritorio"; GroupDescription: "Accesos directos:"

[InstallDelete]
; Las versiones viejas se llamaban MidiSky y "Midi Cat" (con espacio)
Type: filesandordirs; Name: "{commoncf64}\VST3\MidiSky.vst3"
Type: filesandordirs; Name: "{commoncf64}\VST3\Midi Cat.vst3"
Type: filesandordirs; Name: "{autopf}\Midi Cat"
Type: files; Name: "{autoprograms}\Midi Cat.lnk"
Type: filesandordirs; Name: "{autoprograms}\Midi Cat"
Type: files; Name: "{autodesktop}\Midi Cat.lnk"
Type: filesandordirs; Name: "{autopf}\MidiSky"
Type: filesandordirs; Name: "{autoprograms}\MidiSky"
Type: files; Name: "{autodesktop}\MidiSky.lnk"

[Files]
Source: "{#ArtDir}\VST3\MidiCat.vst3\*"; DestDir: "{commoncf64}\VST3\MidiCat.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#ArtDir}\Standalone\MidiCat.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\LEEME-Windows.txt"; DestDir: "{app}"; DestName: "LEEME.txt"; Flags: ignoreversion

[Icons]
Name: "{group}\MidiCat"; Filename: "{app}\MidiCat.exe"
Name: "{group}\Desinstalar MidiCat"; Filename: "{uninstallexe}"
Name: "{autodesktop}\MidiCat"; Filename: "{app}\MidiCat.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\MidiCat.exe"; Description: "Abrir la app MidiCat ahora"; Flags: nowait postinstall skipifsilent unchecked

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\MidiCat.vst3"
