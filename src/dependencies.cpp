// All Typheus dependencies are built separately into a static library.
// This is done in order to not build them every time the rest builds.

// Optick
#ifdef _PROFILE
#include "./core/base.hpp"
#include "./core/profile.hpp"  // this has optick defines

#include "./third_party/optick/optick.h"
#include "./third_party/optick/optick.config.h"
#include "./third_party/optick/optick_capi.h"
#include "./third_party/optick/optick_common.h"
#include "./third_party/optick/optick_core.h"
#include "./third_party/optick/optick_core.platform.h"
#include "./third_party/optick/optick_core.win.h"
#include "./third_party/optick/optick_gpu.h"
#include "./third_party/optick/optick_memory.h"
#include "./third_party/optick/optick_message.h"
#include "./third_party/optick/optick_miniz.h"
#include "./third_party/optick/optick_serialization.h"
#include "./third_party/optick/optick_server.h"

#include "./third_party/optick/optick_capi.cpp"
#include "./third_party/optick/optick_core.cpp"
#include "./third_party/optick/optick_gpu.cpp"
#include "./third_party/optick/optick_message.cpp"
#include "./third_party/optick/optick_miniz.cpp"
#include "./third_party/optick/optick_serialization.cpp"
#include "./third_party/optick/optick_server.cpp"
#endif

// Vulkan Memory Allocator
#define VMA_IMPLEMENTATION
#include "vma/vk_mem_alloc.h"

// ImGui
#include "./third_party/imgui/imgui.cpp"
#include "./third_party/imgui/imgui_draw.cpp"
#include "./third_party/imgui/imgui_tables.cpp"
#include "./third_party/imgui/imgui_widgets.cpp"
#include "./third_party/imgui/imgui_demo.cpp"
#include "./third_party/imgui/backends/imgui_impl_win32.cpp"
#include "./third_party/imgui/backends/imgui_impl_vulkan.cpp"
