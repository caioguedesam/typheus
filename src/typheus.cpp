// Main typheus compilation unit
// This contains all typheus headers and source files to compile in unity build

// Dependency header files
#include "glad/glad.h"
//#define STB_IMAGE_IMPLEMENTATION
//#include "stb_image.h"

// Header files
#include "core/base.hpp"
#include "core/time.hpp"
#include "core/math.hpp"
#include "core/file.hpp"
#include "core/input.hpp"
#include "core/window.hpp"

#include "render/renderer.hpp"

#include "app.hpp"

// Dependency source files
#include "glad/glad.c"
#if _PROFILE
#include "tracy/TracyClient.cpp"
#endif

// Source files
#include "core/base.cpp"
#include "core/time.cpp"
#include "core/math.cpp"
#include "core/file.cpp"
#include "core/input.cpp"
#include "core/window.cpp"

#include "render/renderer.cpp"

#include "app.cpp"
