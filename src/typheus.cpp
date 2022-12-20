// Main typheus compilation unit
// This contains all typheus headers and source files to compile in unity build

// ===============================================================
// [DEPENDENCIES]
#define STB_IMAGE_IMPLEMENTATION
#define CGLTF_IMPLEMENTATION

// Header files
#if _PROFILE
#include "tracy/Tracy.hpp"
#endif

#include "glad/glad.h"
#include "stb_image.h"
#include "cgltf.h"

#undef STB_IMAGE_IMPLEMENTATION
#undef CGLTF_IMPLEMENTATION

// Source files
#if _PROFILE
#include "TracyClient.cpp"
#endif
#include "glad/glad.c"

// ===============================================================
// [PROJECT]
// Header files
#include "core/base.hpp"
#include "core/time.hpp"
#include "core/math.hpp"
#include "core/file.hpp"
#include "core/input.hpp"
#include "core/window.hpp"

#include "render/renderer.hpp"

#include "app.hpp"

// Source files
#include "core/base.cpp"
#include "core/time.cpp"
#include "core/math.cpp"
#include "core/file.cpp"
#include "core/input.cpp"
#include "core/window.cpp"

#include "render/renderer.cpp"

#include "app.cpp"
