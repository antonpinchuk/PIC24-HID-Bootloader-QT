[Dirs]
Name: "{app}\platforms"
Name: "{app}\updates"

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

[Setup]
AppName=Berserker Modded Controller Software
AppVersion=0.0.1.1401301400
DefaultDirName={pf}\viking-one\
OutputBaseFilename=viking-one-setup
AllowRootDirectory=True
