@echo OFF

REM Build tags file
ctags -R

REM Check if build tool is on path
REM >nul, 2>nul will remove the output text from the where command
where cl.exe >nul 2>nul
if %errorlevel%==1 call msvc86.bat

REM Drop compilation files into build folder
IF NOT EXIST ..\bin mkdir ..\bin
pushd ..\bin

set GLEW=..\extern\glew-1.13.0
set GLFW=..\extern\glfw-3.2.bin.WIN32
set STB=..\extern\stb-master
set OAL=..\extern\openal-soft

REM EHsc enables exception handling
REM MD uses dynamic runtime library
REM Zi enables debug data
set compileFlags=/EHsc /MD /Zi /W3

REM Include directories
set includeFlags=/I ..\src\include /I %GLEW%\include /I %GLFW%\include /I %STB%\include /I %OAL%\include

REM Link libraries
set linkLibraries=/link opengl32.lib %GLFW%\lib-vc2015\glfw3.lib %GLEW%\lib\Release\Win32\glew32s.lib %OAL%\lib\Win32\OpenAL32.lib gdi32.lib user32.lib shell32.lib
set ignoreLibraries=/NODEFAULTLIB:"libc.lib" /NODEFAULTLIB:"libcmt.lib" /NODEFAULTLIB:"libcd.lib" /NODEFAULTLIB:"libcmtd.lib" /NODEFAULTLIB:"msvcrtd.lib"

cl %compileFlags%  ..\src\*.c %includeFlags% %linkLibraries% %ignoreLibraries% /OUT:"Dengine.exe"
REM /SUBSYSTEM:CONSOLE

popd
