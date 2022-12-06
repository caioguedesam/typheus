@echo off
setlocal enabledelayedexpansion

set cc_flags=
for /f "delims=" %%x in (compile_flags.txt) do (set cc_flags=!cc_flags! %%x)

set "l_flags=-luser32.lib -lopengl32.lib -lgdi32.lib"

rem clang!cc_flags! --debug -O0 ../src/*.cpp !l_flags! --output=debug/sol.exe
clang!cc_flags! -D_DEBUG --debug -O0 ../src/main.cpp !l_flags! --output=debug/sol.exe

endlocal
