@echo off
setlocal enabledelayedexpansion

set cc_flags=
for /f "delims=" %%x in (compile_flags.txt) do (set cc_flags=!cc_flags! %%x)

set "l_flags=-luser32.lib -lopengl32.lib -lgdi32.lib"

clang!cc_flags! -D_DEBUG --debug -O0 -c ../src/app/dependencies.cpp --output=lib/dependencies_d.o

lib lib/dependencies_d.o
del "lib\dependencies_d.o"

endlocal
