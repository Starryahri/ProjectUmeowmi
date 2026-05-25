@echo off
REM Launch Unreal Editor with GC verification (UE 5.5 flag is -VERIFYGC, not -gcverify).
set UE_EDITOR=D:\Unreal Engines\UE_5.5\Engine\Binaries\Win64\UnrealEditor.exe
set PROJECT=D:\Game Projects\Unreal\ProjectUmeowmi\ProjectUmeowmi.uproject
set GCLOG=D:\Game Projects\Unreal\ProjectUmeowmi\Saved\Logs\GCVerify.log

echo Starting editor with GC diagnostics...
echo   -VERIFYGC                  (enables gc.VerifyAssumptions in editor)
echo   LogGarbage=Verbose
echo   gc.VerifyAssumptionsOnFullPurge=1
echo   gc.VerifyNoUnreachableObjects=1
echo   Separate log: %GCLOG%
echo.
echo Reproduce crash, then search the log for:
echo   Invalid object
echo   ReferencingObject
echo   LogGarbage Fatal
echo.

start "" "%UE_EDITOR%" "%PROJECT%" -VERIFYGC -LogCmds="LogGarbage Verbose" -ExecCmds="gc.VerifyAssumptions 1; gc.VerifyAssumptionsOnFullPurge 1; gc.VerifyNoUnreachableObjects 1" -abslog="%GCLOG%"
