; Instalador de MidiSky para Windows (Inno Setup 6)
#ifndef AppVersion
  #define AppVersion "1.0.0"
#endif
#define ArtDir "..\..\build\MidiSky_artefacts\Release"

[Setup]
AppId={{8F3C2A51-6E2B-4D7A-9C1E-5B7A2D9E4F10}
AppName=MidiSky
AppVersion={#AppVersion}
AppVerName=MidiSky {#AppVersion}
AppPublisher=Joey Ocn
AppPublisherURL=https://github.com/JoeyOcn/LumaKeys
DefaultDirName={autopf}\MidiSky
DefaultGroupName=MidiSky
DisableProgramGroupPage=yes
DisableDirPage=yes
OutputDir=..\..\dist
OutputBaseFilename=MidiSky-Instalador-Windows
Compression=lzma2
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
WizardStyle=modern
ShowLanguageDialog=no
UninstallDisplayName=MidiSky
UninstallDisplayIcon={app}\MidiSky.exe
CloseApplications=yes

[Languages]
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"

[Messages]
spanish.WelcomeLabel2=Esto instala MidiSky en tu computadora:%n%n• El plugin VST3 en la carpeta de plugins de Windows, para que Ableton lo encuentre solo.%n• La app MidiSky para usarla sin Ableton.%n%nSi Ableton está abierto, ciérralo antes de continuar.
spanish.FinishedLabel=MidiSky quedó instalado.%n%nAbre Ableton y búscalo en Plug-Ins > VST3 > Joey Ocn. Si no aparece: Opciones > Preferencias > Plug-Ins, activa "Usar carpeta del sistema VST3" y dale "Reescanear".

[Tasks]
Name: "desktopicon"; Description: "Crear un acceso directo de la app en el escritorio"; GroupDescription: "Accesos directos:"

[Files]
Source: "{#ArtDir}\VST3\MidiSky.vst3\*"; DestDir: "{commoncf64}\VST3\MidiSky.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#ArtDir}\Standalone\MidiSky.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\LEEME-Windows.txt"; DestDir: "{app}"; DestName: "LEEME.txt"; Flags: ignoreversion

[Icons]
Name: "{group}\MidiSky"; Filename: "{app}\MidiSky.exe"
Name: "{group}\Desinstalar MidiSky"; Filename: "{uninstallexe}"
Name: "{autodesktop}\MidiSky"; Filename: "{app}\MidiSky.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\MidiSky.exe"; Description: "Abrir la app MidiSky ahora"; Flags: nowait postinstall skipifsilent unchecked

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\MidiSky.vst3"
