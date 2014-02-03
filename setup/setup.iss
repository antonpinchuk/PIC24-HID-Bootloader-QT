[Dirs]
Name: "{app}\updates"
Name: "{app}\platforms"

[Files]
Source: "D:\Work\Viking\setup\bin\app.ico"; DestDir: "{app}"; Flags: ignoreversion
Source: "D:\Work\Viking\setup\bin\icudt51.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "D:\Work\Viking\setup\bin\icuin51.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "D:\Work\Viking\setup\bin\icuuc51.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "D:\Work\Viking\setup\bin\libgcc_s_dw2-1.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "D:\Work\Viking\setup\bin\libstdc++-6.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "D:\Work\Viking\setup\bin\libwinpthread-1.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "D:\Work\Viking\setup\bin\Qt5Core.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "D:\Work\Viking\setup\bin\Qt5Gui.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "D:\Work\Viking\setup\bin\Qt5Network.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "D:\Work\Viking\setup\bin\Qt5Widgets.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "D:\Work\Viking\setup\bin\viking-one.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "D:\Work\Viking\setup\bin\viking-upd.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "D:\Work\Viking\setup\bin\platforms\qminimal.dll"; DestDir: "{app}\platforms"; Flags: ignoreversion
Source: "D:\Work\Viking\setup\bin\platforms\qoffscreen.dll"; DestDir: "{app}\platforms"; Flags: ignoreversion
Source: "D:\Work\Viking\setup\bin\platforms\qwindows.dll"; DestDir: "{app}\platforms"; Flags: ignoreversion

[Setup]
AppName=Berserker Modded Controller Software
AppVersion=0.0.2.1402031830
DefaultDirName={pf}\viking-one\
OutputBaseFilename=viking-one-setup
AllowRootDirectory=True
DefaultGroupName=Berserker Modded Controller

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"
Name: "autorunicon"; Description: "Run automatically at log on"; GroupDescription: "{cm:AdditionalIcons}"

[Icons]
Name: "{group}\viking one"; Filename: "{app}\viking-one.exe"; WorkingDir: "{app}"; IconFilename: "{app}\app.ico"; IconIndex: 0
Name: "{group}\Uninstall viking one"; Filename: "{uninstallexe}"
Name: "{commondesktop}\viking one"; Filename: "{app}\viking-one.exe"; IconFilename: "{app}\app.ico"; IconIndex: 0; Tasks: desktopicon 
Name: "{commonstartup}\viking one"; Filename: "{app}\viking-one.exe"; IconFilename: "{app}\app.ico"; IconIndex: 0; Tasks: autorunicon 

[Run]
Filename: "{app}\viking-one.exe"; Description: "Launch application"; Flags: postinstall nowait skipifsilent
