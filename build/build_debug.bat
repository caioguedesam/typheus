@echo off
setlocal enabledelayedexpansion

set cc_flags=
for /f "delims=" %%x in (compile_flags.txt) do (set cc_flags=!cc_flags! %%x)

set "l_flags=-luser32.lib -lopengl32.lib -lgdi32.lib -llib/dependencies_d.lib"

clang!cc_flags! -ftime-trace=build_trace.json -D_DEBUG --debug -O0 -include-pch ../src/engine/common/stdafx.pch ../src/app/main.cpp !l_flags! -Wl,-nodefaultlib:libcmt -lmsvcrtd.lib --output=debug/app.exe

endlocal
