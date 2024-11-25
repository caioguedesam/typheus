# Typheus engine build script.
# TODO(caio): PROFILE builds

import sys
import subprocess
import time
import glob
import os

typheus_lib_name = "ty"
typheus_dep_lib_name = "ty_dependencies_only"

def clean(output_dir):
    dir_bs = output_dir.replace('/', '\\')
    object_files = glob.glob(f'{dir_bs}\\*.obj')
    for file in object_files:
        subprocess.run(f'del "{file}"', shell=True)

def build_pch(output_dir, cc_flags, optimize=False):
    print(f'Starting precompiled header build...')
    build_command = f'clang {cc_flags} -c ./src/stdafx.hpp -emit-pch'
    if optimize:
        build_command += f' -Ofast --output={output_dir}/stdafx_r.hpp.pch'
    else:
        build_command += f' -O0 --output={output_dir}/stdafx_d.hpp.pch'

    start = time.time()
    subprocess.run(build_command, shell=True, stdout=subprocess.DEVNULL)
    end = time.time()

    print(f'Finished precompiled header build in {end - start} seconds.')

def build_engine_dependencies(output_dir, cc_flags):
    # Debug dependencies are compiled as release for speed
    print(f'Starting engine dependencies build...')
    build_command = f'clang {cc_flags} -Ofast -Wno-nullability-completeness -c'
    build_command += f' -include-pch {output_dir}/stdafx_r.hpp.pch'
    build_command += f' ./src/dependencies.cpp'
    build_command += f' -fms-runtime-lib=dll'
    build_command += f' --output={output_dir}/{typheus_dep_lib_name}.obj'

    start = time.time()
    subprocess.run(build_command, shell=True, stdout=subprocess.DEVNULL)
    subprocess.run(f'lib {output_dir}/{typheus_dep_lib_name}.obj', shell=True)
    end = time.time()

    print(f'Finished engine dependencies build in {end - start} seconds.')

def build_engine(output_dir, build_type, cc_flags):
    build_command = f'clang {cc_flags} -c'
    if build_type == 'd':
        build_command += f' --debug -O0'
        build_command += f' -DTY_DEBUG=1'
        build_command += f' -include-pch {output_dir}/stdafx_d.hpp.pch'
    elif build_type == 'r':
        build_command += f' -Ofast'
        build_command += f' -DTY_NDEBUG=1'
        build_command += f' -include-pch {output_dir}/stdafx_r.hpp.pch'
    build_command += f' ./src/main.cpp'
    build_command += f' -fms-runtime-lib=dll'
    build_command += f' --output={output_dir}/{typheus_lib_name}.obj'
    
    print('Starting engine build...')
    start = time.time()

    subprocess.run(build_command, shell=True)

    link_command = f'lib /OUT:{output_dir}/{typheus_lib_name}.lib {output_dir}/{typheus_lib_name}.obj'
    link_command += f' {output_dir}/{typheus_dep_lib_name}.lib'
    link_command += f' user32.lib gdi32.lib'
    link_command += f' C:/VulkanSDK/1.3.239.0/Lib/vulkan-1.lib'
    # TODO(caio): Including shader compiler code increases library size in almost 80MB.
    # Should probably find some other solution for small executables.
    link_command += f' C:/VulkanSDK/1.3.239.0/Lib/shaderc_combined.lib'
    link_command += f' /IGNORE:4006'
    subprocess.run(link_command, shell=True)

    end = time.time()
    print(f'Finished building typheus in {end - start} seconds')

# Build type
build_type = ''
output_dir = ''
if '-d' in sys.argv:
    build_type = 'd'    # Debug
    output_dir = './build/debug'
elif '-r' in sys.argv:
    build_type = 'r'    # Release
    output_dir = './build/release'

output_dir = output_dir.replace('\\', '/')
os.makedirs(output_dir, exist_ok=True)

# Compiler flags
f = open('compile_flags.txt')
cc_flags = ' '.join(f.read().splitlines())
f.close()

if '--full' in sys.argv:
    print('full build')
    build_pch(output_dir, cc_flags, False)
    build_pch(output_dir, cc_flags, True)
    build_engine_dependencies(output_dir, cc_flags)

build_engine(output_dir, build_type, cc_flags)
clean(output_dir)
