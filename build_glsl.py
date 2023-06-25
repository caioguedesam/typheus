import os
import sys
import subprocess
import time
from colorama import Fore, Back, Style

shader_dir = './resources/shaders'
shader_output = './build'

def list_shader_files(path):
    result = []
    for root, dirs, files in os.walk(path):
        for file in files:
            result += [file]
        break
    return result

shader_files = list_shader_files(shader_dir)
debug = True if '-d' in sys.argv else False

print('Starting shader build...')
start = time.time()
# subprocess.run(build_command, shell=True)
for file in shader_files:
    command = f'glslc {shader_dir}/{file}'
    if debug: command += ' -g -O0'
    else: command += ' -O'
    file_no_ext = os.path.splitext(file)[0]
    file_ext = os.path.splitext(file)[1][1:]
    command += f' -o {shader_output}/{file_no_ext}_{file_ext}.spv'
    print(Fore.YELLOW + f'Building {file}...')
    subprocess.run(command, shell=True)

end = time.time()
print(Style.RESET_ALL)
print(f'Finished shader build in {end - start} seconds')
