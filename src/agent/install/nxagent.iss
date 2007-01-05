; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

[Setup]
AppName=NetXMS Agent
AppVerName=NetXMS Agent 0.2.15-rc4
AppVersion=0.2.15-rc4
AppPublisher=NetXMS Team
AppPublisherURL=http://www.netxms.org
AppSupportURL=http://www.netxms.org
AppUpdatesURL=http://www.netxms.org
DefaultDirName=C:\NetXMS
DefaultGroupName=NetXMS Agent
AllowNoIcons=yes
OutputBaseFilename=nxagent-0.2.15-rc4
Compression=lzma
SolidCompression=yes
LanguageDetectionMethod=none
LicenseFile=..\..\..\copying

[Files]
Source: "..\..\libnetxms\Release\libnetxms.dll"; DestDir: "{app}\bin"; BeforeInstall: StopService; Flags: ignoreversion
Source: "..\core\Release\nxagentd.exe"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "..\subagents\winnt\Release\winnt.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "..\subagents\win9x\Release\win9x.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "..\subagents\winperf\Release\winperf.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "..\subagents\ping\Release\ping.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "..\subagents\portCheck\Release\portcheck.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "..\subagents\ecs\Release\ecs.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "..\subagents\logscan\Release\logscan.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "..\subagents\odbcquery\Release\odbcquery.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion
;Source: "..\subagents\rtmonitor\Release\rtmonitor.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "..\subagents\ups\Release\ups.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "..\..\..\contrib\nxagentd.conf-dist"; DestDir: "{app}\etc"; Flags: ignoreversion
Source: "..\..\install\windows\files\libeay32.dll"; DestDir: "{app}\bin"; Flags: ignoreversion

[Dirs]
Name: "{app}\etc"
Name: "{app}\var"

[Run]
Filename: "{app}\bin\nxagentd.exe"; Parameters: "-Z ""{app}\etc\nxagentd.conf"" ""{code:GetMasterServer}"" {{syslog} ""{app}\var"" {code:GetSubagentList}"; WorkingDir: "{app}\bin"; StatusMsg: "Creating agent's config..."; Flags: runhidden
Filename: "{app}\bin\nxagentd.exe"; Parameters: "-c ""{app}\etc\nxagentd.conf"" -I"; WorkingDir: "{app}\bin"; StatusMsg: "Installing service..."; Flags: runhidden
Filename: "{app}\bin\nxagentd.exe"; Parameters: "-s"; WorkingDir: "{app}\bin"; StatusMsg: "Starting service..."; Flags: runhidden

[UninstallRun]
Filename: "{app}\bin\nxagentd.exe"; Parameters: "-S"; StatusMsg: "Stopping service..."; RunOnceId: "StopService"; Flags: runhidden
Filename: "{app}\bin\nxagentd.exe"; Parameters: "-R"; StatusMsg: "Uninstalling service..."; RunOnceId: "DelService"; Flags: runhidden

[Code]
Var
  ServerSelectionPage: TInputQueryWizardPage;
  SubagentSelectionPage: TInputOptionWizardPage;
  serverName, sbPing, sbPortCheck, sbWinPerf, sbUPS: String;

Procedure StopService;
Var
  strExecName : String;
  iResult : Integer;
Begin
  strExecName := ExpandConstant('{app}\bin\nxagentd.exe');
  If FileExists(strExecName) Then
  Begin
    Exec(strExecName, '-S', ExpandConstant('{app}\bin'), 0, ewWaitUntilTerminated, iResult);
  End;
End;

Function BoolToStr(Val: Boolean): String;
Begin
  If Val Then
    Result := 'TRUE'
  Else
    Result := 'FALSE';
End;

Function StrToBool(Val: String): Boolean;
Begin
  If Val = 'TRUE' Then
    Result := TRUE
  Else
    Result := FALSE;
End;

Function InitializeSetup(): Boolean;
Var
  i, nCount : Integer;
  param : String;
Begin
  // Check if we are running on 64-bit Windows
  If ProcessorArchitecture = paX64 Then Begin
    If MsgBox('You are trying to install 32-bit version of NetXMS agent on 64-bit Windows. It is recommended to install 64-bit version instead. Do you really wish to continue installation?', mbConfirmation, MB_YESNO) = IDYES Then
      Result := TRUE
    Else
      Result := FALSE;
  End Else Begin
    Result := TRUE;
  End;
  
  // Empty values for installation data
  serverName := '';
  sbPing := 'FALSE';
  sbPortCheck := 'FALSE';
  sbWinPerf := 'TRUE';
  sbUPS := 'FALSE';
  
  // Parse command line parameters
  nCount := ParamCount;
  For i := 1 To nCount Do Begin
    param := ParamStr(i);

    If Pos('/SERVER=', param) = 1 Then Begin
      serverName := param;
      Delete(serverName, 1, 8);
    End;

    If Pos('/SUBAGENT=', param) = 1 Then Begin
      Delete(param, 1, 10);
      param := Uppercase(param);
      If param = 'PING' Then
        sbPing := 'TRUE';
      If param = 'PORTCHECK' Then
        sbPortCheck := 'TRUE';
      If param = 'WINPERF' Then
        sbWinPerf := 'TRUE';
      If param = 'UPS' Then
        sbUPS := 'TRUE';
    End;

    If Pos('/NOSUBAGENT=', param) = 1 Then Begin
      Delete(param, 1, 12);
      param := Uppercase(param);
      If param = 'PING' Then
        sbPing := 'FALSE';
      If param = 'PORTCHECK' Then
        sbPortCheck := 'FALSE';
      If param = 'WINPERF' Then
        sbWinPerf := 'FALSE';
      If param = 'UPS' Then
        sbUPS := 'FALSE';
    End;
  End;
End;

Procedure InitializeWizard;
Begin
  ServerSelectionPage := CreateInputQueryPage(wpSelectTasks,
    'NetXMS Server', 'Select your management server.',
    'Please enter host name or IP address of your NetXMS server.');
  ServerSelectionPage.Add('NetXMS server:', False);
  ServerSelectionPage.Values[0] := GetPreviousData('MasterServer', serverName)

  SubagentSelectionPage := CreateInputOptionPage(ServerSelectionPage.Id,
    'Subagent Selection', 'Select desired subagents.',
    'Please select additional subagents you wish to load.', False, False);
  SubagentSelectionPage.Add('ICMP Pinger Subagent - ping.nsm');
  SubagentSelectionPage.Add('Port Checker Subagent - portcheck.nsm');
  SubagentSelectionPage.Add('Windows Performance Subagent - winperf.nsm');
  SubagentSelectionPage.Add('UPS Monitoring Subagent - ups.nsm');
  SubagentSelectionPage.Values[0] := StrToBool(GetPreviousData('Subagent_PING', sbPing));
  SubagentSelectionPage.Values[1] := StrToBool(GetPreviousData('Subagent_PORTCHECK', sbPortCheck));
  SubagentSelectionPage.Values[2] := StrToBool(GetPreviousData('Subagent_WINPERF', sbWinPerf));
  SubagentSelectionPage.Values[3] := StrToBool(GetPreviousData('Subagent_UPS', sbUPS));
End;

Procedure RegisterPreviousData(PreviousDataKey: Integer);
Begin
  SetPreviousData(PreviousDataKey, 'MasterServer', ServerSelectionPage.Values[0]);
  SetPreviousData(PreviousDataKey, 'Subagent_PING', BoolToStr(SubagentSelectionPage.Values[0]));
  SetPreviousData(PreviousDataKey, 'Subagent_PORTCHECK', BoolToStr(SubagentSelectionPage.Values[1]));
  SetPreviousData(PreviousDataKey, 'Subagent_WINPERF', BoolToStr(SubagentSelectionPage.Values[2]));
  SetPreviousData(PreviousDataKey, 'Subagent_UPS', BoolToStr(SubagentSelectionPage.Values[3]));
End;

Function GetMasterServer(Param: String): String;
Begin
  Result := ServerSelectionPage.Values[0];
End;

Function GetSubagentList(Param: String): String;
Begin
  Result := '';
  If SubagentSelectionPage.Values[0] Then
    Result := Result + 'ping.nsm ';
  If SubagentSelectionPage.Values[1] Then
    Result := Result + 'portcheck.nsm ';
  If SubagentSelectionPage.Values[2] Then
    Result := Result + 'winperf.nsm ';
  If SubagentSelectionPage.Values[3] Then
    Result := Result + 'ups.nsm ';
End;

