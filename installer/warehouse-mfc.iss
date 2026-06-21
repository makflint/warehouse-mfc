; Inno Setup script for the warehouse-mfc DEMO build.
; Bundles the app + the SQL scripts, and installs the two runtime prerequisites
; (Visual C++ runtime for dynamic MFC, and SQL Server LocalDB). The app seeds its
; LocalDB database on first run, so no DB step is needed here.
;
; Build:  "%LOCALAPPDATA%\Programs\Inno Setup 6\ISCC.exe" installer\warehouse-mfc.iss
; Assets (gitignored) must be present under installer\assets\:
;   - vc_redist.x64.exe   (from the VS 2022 VC\Redist folder)
;   - SqlLocalDB.msi      (SQL Server 2022 LocalDB standalone installer)

#define MyAppName "warehouse-mfc"
#define MyAppVersion "1.0.0-demo"
#define MyAppExe "app.exe"

[Setup]
AppId={{A4F2C7E0-3D1B-4E9A-9C2F-1A2B3C4D5E6F}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher=Maciej Krzeminski
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
OutputDir=Output
OutputBaseFilename=warehouse-mfc-setup
Compression=lzma2
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
WizardStyle=modern

[Tasks]
Name: "desktopicon"; Description: "Utworz skrot na pulpicie"; GroupDescription: "Skroty:"

[Files]
Source: "..\app\x64\Release\app.exe"; DestDir: "{app}"; Flags: ignoreversion
; Offline Polish speech model for voice commands (whisper.cpp, ~141 MB). Loaded from
; next to the exe at runtime; whisper is statically linked, so no extra DLLs ship.
Source: "..\models\ggml-base.bin"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\db\01_schema.sql"; DestDir: "{app}\db"; Flags: ignoreversion
Source: "..\db\02_seed.sql"; DestDir: "{app}\db"; Flags: ignoreversion
Source: "assets\vc_redist.x64.exe"; DestDir: "{tmp}"; Flags: deleteafterinstall
Source: "assets\SqlLocalDB.msi"; DestDir: "{tmp}"; Flags: deleteafterinstall

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExe}"
Name: "{group}\Odinstaluj {#MyAppName}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExe}"; Tasks: desktopicon

[Run]
Filename: "{tmp}\vc_redist.x64.exe"; Parameters: "/install /quiet /norestart"; \
    StatusMsg: "Instalowanie Visual C++ Runtime..."; Flags: waituntilterminated
Filename: "msiexec.exe"; Parameters: "/i ""{tmp}\SqlLocalDB.msi"" /qn IACCEPTSQLLOCALDBLICENSETERMS=YES"; \
    StatusMsg: "Instalowanie SQL Server LocalDB..."; Flags: waituntilterminated
Filename: "{app}\{#MyAppExe}"; Description: "Uruchom {#MyAppName}"; \
    Flags: nowait postinstall skipifsilent
