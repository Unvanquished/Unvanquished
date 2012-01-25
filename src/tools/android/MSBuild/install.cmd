@echo off

set MsBuildCppDir=

echo.

rem On 64-bit machines, Visual Studio 2010 and MsBuild is in the (x86) directory. So try that last.

if exist "%ProgramFiles%" set MsBuildCppDir=%ProgramFiles%\MSBuild\Microsoft.Cpp\v4.0\Platforms
if exist "%ProgramFiles(x86)%" set MsBuildCppDir=%ProgramFiles(x86)%\MSBuild\Microsoft.Cpp\v4.0\Platforms



Rem This will fail if we don't have admin privs.
mkdir "%windir%\system32\test" 2>nul
IF ERRORLEVEL 1 (
	
	Echo.This batch file needs to be ran with administrative privileges. Since it copies files to the \Program Files directory.
	Pause
	Goto cleanup

) else (

	rem Remove that test directory we just made
	rmdir "%windir%\system32\test"

	if not exist "%MsBuildCppDir%" (

		Echo.Failed to find the MsBuild directories that should have been installed with 'Visual Studio 2010'.
		Echo."%MsBuildCppDir%"
		Pause
		goto cleanup
		
	)


	if exist "%MsBuildCppDir%\Android" (
		
		Echo.An Android Cpp MsBuild toolset already exists.
		Echo.Continuing will delete the version already installed to this directory:

		rd "%MsBuildCppDir%\Android" /s
		pause
		if exist "%MsBuildCppDir%\Android" (
			goto cleanup
		)

		echo.
	)

	md "%MsBuildCppDir%\Android"

	echo.Installing Android MSBuild files:
	cd /d %~dp0
	xcopy "Android\*.*" "%MsBuildCppDir%\Android" /E

	if errorlevel 1 (
	
		echo.Problem with copying
		Pause
		goto cleanup

	)

	echo.
	echo.Done! You will need to close and re-open existing instances of Visual Studio.
	Pause

)

:cleanup
set MsBuildCppDir=
