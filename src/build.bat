@echo OFF
REM Drop compilation files into build folder
IF NOT EXIST ..\bin mkdir ..\bin
pushd ..\bin

set GLEW=..\extern\glew-1.13.0
set GLFW=..\extern\glfw-3.1.2.bin.WIN32
set STB=..\extern\stb-master
set GLM=..\extern\glm-0.9.7.5

REM EHsc enables exception handling
REM MD uses dynamic runtime library
REM Zi enables debug data
set compileFlags=/EHsc /MD /Zi

REM Include directories
set includeFlags=/I ..\src\include /I %GLEW%\include /I %GLFW%\include /I %STB%\include /I %GLM%\include

REM Link libraries
set linkLibraries=/link opengl32.lib %GLFW%\lib-vc2015\glfw3.lib %GLEW%\lib\Release\Win32\glew32s.lib gdi32.lib user32.lib shell32.lib

cl %compileFlags%  ..\src\*.cpp %includeFlags% %linkLibraries%
REM /SUBSYSTEM:CONSOLE

popd
