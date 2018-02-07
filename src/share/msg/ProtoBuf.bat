@echo off

set ProtoGen=protoc.exe
set Output=..\..\client\Messages

for /f "delims=" %%i in ('dir /b proto "*.proto"') do (
	echo %%i
	%ProtoGen% --csharp_out=%Output% %%i
)

pause