#TODO(caio): Profile build
#TODO(caio): Dependencies build
import sys
import subprocess

# Build type
build_type = None
if '-d' in sys.argv:
    build_type = 'd'    # Debug
elif '-r' in sys.argv:
    build_type = 'r'    # Release
elif '-p' in sys.argv:
    build_type = 'p'    # Profile
full_build = '--full' in sys.argv

# Compiler flags
f = open('compile_flags.txt')
cc_flags = ' '.join(f.read().splitlines())
f.close()

# PCH build (full build only)
if full_build:
    build_command_pch = f'clang {cc_flags} -c ./src/engine/stdafx.hpp -emit-pch -o ./build/stdafx.pch'
    print(build_command_pch)
    subprocess.run(build_command_pch, shell=True)

# Linker flags
l_flags = '-luser32.lib -lopengl32.lib -lgdi32.lib' #TODO(caio): add -llib/dependencies_d(r).lib

# Dependencies build (full build only)
#TODO(caio): No dependencies yet

# Build command (clang)
build_command = ''
if build_type == 'd':
    build_command = f'clang {cc_flags} -D_DEBUG --debug -O0 -include-pch ./build/stdafx.pch ./src/app/main.cpp {l_flags} -Wl,-nodefaultlib:libcmt -lmsvcrtd.lib --output=./build/debug/app.exe'
elif build_type == 'r':     # No PCH on release builds
    build_command = f'clang {cc_flags} -D_RELEASE -Ofast ./src/app/main.cpp {l_flags} --output=./build/release/app.exe'

subprocess.run(build_command, shell=True)
