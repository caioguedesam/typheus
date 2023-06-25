// All Typheus dependencies are built separately into a static library.
// This is done in order to not build them every time the rest builds.

// Optick
#ifdef _PROFILE
#include "engine/core/base.hpp"
#include "engine/core/profile.hpp"  // this has optick defines

#include "optick.h"
#include "optick.config.h"
#include "optick_capi.h"
#include "optick_common.h"
#include "optick_core.h"
#include "optick_core.platform.h"
#include "optick_core.win.h"
#include "optick_gpu.h"
#include "optick_memory.h"
#include "optick_message.h"
#include "optick_miniz.h"
#include "optick_serialization.h"
#include "optick_server.h"

#include "optick_capi.cpp"
#include "optick_core.cpp"
#include "optick_gpu.cpp"
#include "optick_message.cpp"
#include "optick_miniz.cpp"
#include "optick_serialization.cpp"
#include "optick_server.cpp"
#endif

// Vulkan Memory Allocator
#define VMA_IMPLEMENTATION
#include "vma/vk_mem_alloc.h"

// ImGui
#include "imgui.cpp"
#include "imgui_draw.cpp"
#include "imgui_tables.cpp"
#include "imgui_widgets.cpp"
#include "imgui_demo.cpp"
#include "backends/imgui_impl_win32.cpp"
#include "backends/imgui_impl_vulkan.cpp"
