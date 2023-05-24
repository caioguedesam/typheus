// All Typheus dependencies are built separately into a static library.
// This is done in order to not build them every time the rest builds.

// [DEPENDENCIES]
// Implementation defines
//#define STB_IMAGE_IMPLEMENTATION
//#define FAST_OBJ_IMPLEMENTATION
//#define IMGUI_IMPL_OPENGL_LOADER_CUSTOM     // Need this for unity build since
//                                             //ImGui has own OpenGL function loader

// Header files
//#if _PROFILE
//#include "tracy/Tracy.hpp"
//#endif
//#include "glad/glad.h"
//#include "stb_image.h"
//#include "fast_obj.h"
//#include "imgui.h"
//#include "backends/imgui_impl_win32.h"
//#include "backends/imgui_impl_opengl3.h"

//#undef STB_IMAGE_IMPLEMENTATION
//#undef FAST_OBJ_IMPLEMENTATION
// Source files
//#if _PROFILE
//#include "TracyClient.cpp"
//#endif
//#include "glad/glad.c"
//#include "imgui.cpp"
//#include "imgui_demo.cpp"
//#include "imgui_draw.cpp"
//#include "imgui_tables.cpp"
//#include "imgui_widgets.cpp"
//#include "backends/imgui_impl_win32.cpp"
//#include "backends/imgui_impl_opengl3.cpp"
//#undef IMGUI_IMPL_OPENGL_LOADER_CUSTOM
