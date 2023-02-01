@echo off
setlocal enabledelayedexpansion

set cc_flags=
for /f "delims=" %%x in (compile_flags.txt) do (set cc_flags=!cc_flags! %%x)

clang!cc_flags! -c ../src/engine/common/stdafx.hpp -emit-pch -o ../src/engine/common/stdafx.pch

endlocal
