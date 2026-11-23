; Instalador de Midi Cat para Windows (Inno Setup 6)
#ifndef AppVersion
  #define AppVersion "1.3.0"
#endif
#define ArtDir "..\..\build\MidiCat_artefacts\Release"

[Setup]
; Mismo AppId que la versión vieja (MidiSky): así la reemplaza en vez de quedar las dos
AppId={{8F3C2A51-6E2B-4D7A-9C1E-5B7A2D9E4F10}
AppName=Midi Cat
AppVersion={#AppVersion}
AppVerName=Midi Cat {#AppVersion}
AppPublisher=Joey Ocn
AppPublisherURL=https://github.com/JoeyOcn/LumaKeys
DefaultDirName={autopf}\Midi Cat
DefaultGroupName=Midi Cat
UsePreviousAppDir=no
UsePreviousGroup=no
DisableProgramGroupPage=yes
DisableDirPage=yes
OutputDir=..\..\dist
OutputBaseFilename=Midi-Cat-Instalador-Windows
SetupIconFile=MidiCat.ico
Compression=lzma2
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
WizardStyle=modern
ShowLanguageDialog=no
UninstallDisplayName=Midi Cat
UninstallDisplayIcon={app}\Midi Cat.exe
CloseApplications=yes

[Languages]
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"

[Messages]
spanish.WelcomeLabel2=Esto instala Midi Cat en tu computadora:%n%n• El plugin VST3 en la carpeta de plugins de Windows, para que Ableton lo encuentre solo.%n• La app Midi Cat para usarla sin Ableton.%n%nSi tenías la versión vieja (MidiSky), se quita sola.%nSi Ableton está abierto, ciérralo antes de continuar.
spanish.FinishedLabel=Midi Cat quedó instalado.%n%nAbre Ableton y búscalo en Plug-Ins > VST3 > Joey Ocn. Si no aparece: Opciones > Preferencias > Plug-Ins, activa "Usar carpeta del sistema VST3" y dale "Reescanear".

[Tasks]
Name: "desktopicon"; Description: "Crear un acceso directo de la app en el escritorio"; GroupDescription: "Accesos directos:"

[InstallDelete]
; La versión vieja se llamaba MidiSky
Type: filesandordirs; Name: "{commoncf64}\VST3\MidiSky.vst3"
Type: filesandordirs; Name: "{autopf}\MidiSky"
Type: filesandordirs; Name: "{autoprograms}\MidiSky"
Type: files; Name: "{autodesktop}\MidiSky.lnk"

[Files]
Source: "{#ArtDir}\VST3\Midi Cat.vst3\*"; DestDir: "{commoncf64}\VST3\Midi Cat.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#ArtDir}\Standalone\Midi Cat.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\LEEME-Windows.txt"; DestDir: "{app}"; DestName: "LEEME.txt"; Flags: ignoreversion

[Icons]
Name: "{group}\Midi Cat"; Filename: "{app}\Midi Cat.exe"
Name: "{group}\Desinstalar Midi Cat"; Filename: "{uninstallexe}"
Name: "{autodesktop}\Midi Cat"; Filename: "{app}\Midi Cat.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\Midi Cat.exe"; Description: "Abrir la app Midi Cat ahora"; Flags: nowait postinstall skipifsilent unchecked

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\Midi Cat.vst3"
