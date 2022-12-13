@echo off
setlocal enabledelayedexpansion

set cc_flags=
for /f "delims=" %%x in (compile_flags.txt) do (set cc_flags=!cc_flags! %%x)

set "l_flags=-luser32.lib -lopengl32.lib -lgdi32.lib"

clang!cc_flags! -Ofast -D_RELEASE ../src/main.cpp !l_flags! --output=release/sol.exe

endlocal
