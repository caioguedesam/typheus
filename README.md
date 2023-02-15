# Typheus
Typheus is a C++ library for general purpose game/application development. Currently, Typheus' main purpose is to work as a real-time 3D rendering framework on top of OpenGL, to use in game and graphics projects. Additional systems (like audio, animation, networking) may be developed in the future.

Typheus is developed in C++17, and uses OpenGL as a graphics API. In the near future, it will be ported to Vulkan.

## Examples

Here are some screenshots highlighting Typheus' current state:

![Imgur](https://i.imgur.com/OGjm69p.png)
Deferred rendering scene setup with directional light

![Imgur](https://i.imgur.com/KlhyvTt.png)
Point lights

![Imgur](https://i.imgur.com/obsQJFa.png)
Skybox

![Imgur](https://i.imgur.com/WRfBrA1.png)
Directional shadow mapping with PCF

## Build instructions

Soon:tm:

## Current Milestone Goals
- Shadow mapping
- Soft shadows (PCF)
- Unified CMake build pipeline

## Dependencies

Typheus requires a few third party libraries to operate:
- [stb_image.h](https://github.com/nothings/stb/blob/master/stb_image.h)
- [fast_obj](https://github.com/thisistherk/fast_obj)
- [Dear ImGui](https://github.com/ocornut/imgui)
- [Tracy](https://github.com/wolfpld/tracy) (only for profiling builds)
