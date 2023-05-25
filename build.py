#TODO(caio): Profile build
#TODO(caio): Dependencies build
import sys
import subprocess
import time

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
if full_build and build_type == 'd':
    print(f'Starting precompiled header build...')
    start = time.time()
    build_command_pch = f'clang {cc_flags} -c ./src/engine/stdafx.hpp -emit-pch -o ./build/stdafx.pch'
    subprocess.run(build_command_pch, shell=True)
    end = time.time()
    print(f'Finished precompiled header build in {end - start} seconds')

# Linker flags
l_flags = '-luser32.lib -lopengl32.lib -lgdi32.lib'
if build_type == 'p':
    l_flags += ' -lbuild/profile/dependencies.lib'

# Dependencies build (full build only)
#TODO(caio): No dependencies yet
if full_build:
    print(f'Starting dependencies build...')
    start = time.time()
    if build_type == 'p':
        subprocess.run(f'clang {cc_flags} -D_NDEBUG -D_PROFILE -Ofast -c ./src/app/dependencies.cpp --output=./build/profile/dependencies.o', shell=True, stdout=subprocess.DEVNULL)
        subprocess.run('lib ./build/profile/dependencies.o', shell=True)
        subprocess.run('del ".\\build\\profile\\dependencies.o\"', shell=True)
    end = time.time()
    print(f'Finished dependencies build in {end - start} seconds')

# Build command (clang)
build_command = ''
if build_type == 'd':
    build_command = f'clang {cc_flags} -D_DEBUG --debug -O0 -include-pch ./build/stdafx.pch ./src/app/main.cpp {l_flags} -Wl,-nodefaultlib:libcmt -lmsvcrtd.lib --output=./build/debug/app.exe'
elif build_type == 'r':     # No PCH on release builds
    build_command = f'clang {cc_flags} -D_NDEBUG -Ofast ./src/app/main.cpp {l_flags} --output=./build/release/app.exe'
elif build_type == 'p':     # No PCH on profile builds
    build_command = f'clang {cc_flags} -D_NDEBUG -D_PROFILE -Ofast ./src/app/main.cpp {l_flags} --output=./build/profile/app.exe'

print('Starting project build...')
start = time.time()
subprocess.run(build_command, shell=True)
end = time.time()
print(f'Finished project build in {end - start} seconds')
