@echo off
rem Copies the 64-bit FreeGLUT files from C:\freeglut into this project
rem and puts libfreeglut.dll next to the Debug and Release executables.
rem Run it once after downloading the project, and again after a fresh clone.

setlocal
cd /d "%~dp0"
set FG=C:\freeglut

if not exist "%FG%\include\GL\freeglut.h" (
    echo ERROR: %FG%\include\GL\freeglut.h not found.
    goto fail
)
if not exist "%FG%\lib\x64\libfreeglut.dll.a" (
    echo ERROR: %FG%\lib\x64\libfreeglut.dll.a not found.
    goto fail
)
if not exist "%FG%\bin\x64\libfreeglut.dll" (
    echo ERROR: %FG%\bin\x64\libfreeglut.dll not found.
    goto fail
)

if not exist "third_party\freeglut\include\GL" mkdir "third_party\freeglut\include\GL"
if not exist "third_party\freeglut\lib" mkdir "third_party\freeglut\lib"
if not exist "third_party\freeglut\bin" mkdir "third_party\freeglut\bin"
if not exist "bin\Debug" mkdir "bin\Debug"
if not exist "bin\Release" mkdir "bin\Release"

copy /Y "%FG%\include\GL\*.h" "third_party\freeglut\include\GL\" >nul || goto fail
copy /Y "%FG%\lib\x64\libfreeglut.dll.a" "third_party\freeglut\lib\" >nul || goto fail
copy /Y "%FG%\bin\x64\libfreeglut.dll" "third_party\freeglut\bin\" >nul || goto fail
copy /Y "third_party\freeglut\bin\libfreeglut.dll" "bin\Debug\" >nul || goto fail
copy /Y "third_party\freeglut\bin\libfreeglut.dll" "bin\Release\" >nul || goto fail

echo.
echo SUCCESS: FreeGLUT copied. Now open LungiDynamics.cbp and press F9.
echo.
pause
exit /b 0

:fail
echo.
echo SETUP FAILED. Take a screenshot of this window and send it to me.
echo.
pause
exit /b 1
