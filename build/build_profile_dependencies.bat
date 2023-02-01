@echo off
setlocal enabledelayedexpansion

set cc_flags=
for /f "delims=" %%x in (compile_flags.txt) do (set cc_flags=!cc_flags! %%x)

set "l_flags=-luser32.lib -lopengl32.lib -lgdi32.lib"

clang!cc_flags! -Ofast -D_PROFILE -DTRACY_ENABLE -c ../src/app/dependencies.cpp --output=lib/dependencies_p.o

lib lib/dependencies_p.o
del "lib\dependencies_p.o"

endlocal
