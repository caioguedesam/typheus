// ========================================================
// INPUT
// Keyboard and mouse input handling functions.
// @Caio Guedes, 2023
// ========================================================

#pragma once
#include "./base.hpp"
#include "./math.hpp"

namespace ty
{
namespace input
{

// ========================================================
// [KEYS]
// These match Win32 enums, since it's the only supported OS and
// who knows when this will change.
enum InputKey : u32
{
    KEY_INVALID         = 0x00,
    KEY_LMB             = 0x01,
    KEY_RMB             = 0x02,
    KEY_MMB             = 0x03,
    KEY_BACKSPACE       = 0x08,
    KEY_TAB             = 0x09,
    KEY_RETURN          = 0x0D,
    KEY_SHIFT           = 0x10,
    KEY_CTRL            = 0x11,
    KEY_ALT             = 0x12,
    KEY_ESCAPE          = 0x1B,
    KEY_SPACE           = 0x20,
    KEY_ARROW_LEFT      = 0x25,
    KEY_ARROW_UP        = 0x26,
    KEY_ARROW_RIGHT     = 0x27,
    KEY_ARROW_DOWN      = 0x28,
    KEY_0               = 0x30,
    KEY_1               = 0x31,
    KEY_2               = 0x32,
    KEY_3               = 0x33,
    KEY_4               = 0x34,
    KEY_5               = 0x35,
    KEY_6               = 0x36,
    KEY_7               = 0x37,
    KEY_8               = 0x38,
    KEY_9               = 0x39,
    KEY_A               = 0x41,
    KEY_B               = 0x42,
    KEY_C               = 0x43,
    KEY_D               = 0x44,
    KEY_E               = 0x45,
    KEY_F               = 0x46,
    KEY_G               = 0x47,
    KEY_H               = 0x48,
    KEY_I               = 0x49,
    KEY_J               = 0x4A,
    KEY_K               = 0x4B,
    KEY_L               = 0x4C,
    KEY_M               = 0x4D,
    KEY_N               = 0x4E,
    KEY_O               = 0x4F,
    KEY_P               = 0x50,
    KEY_Q               = 0x51,
    KEY_R               = 0x52,
    KEY_S               = 0x53,
    KEY_T               = 0x54,
    KEY_U               = 0x55,
    KEY_V               = 0x56,
    KEY_W               = 0x57,
    KEY_X               = 0x58,
    KEY_Y               = 0x59,
    KEY_Z               = 0x5A,
    //TODO(caio): Add more whenever needed.
};

bool IsKeyDown      (InputKey key);
bool IsKeyUp        (InputKey key);
bool IsKeyJustDown  (InputKey key);
bool IsKeyJustUp    (InputKey key);

// ========================================================
// [MOUSE]

struct MouseState
{
    math::v2i pos;    // Mouse position in pixels starting from TOP-LEFT of application window.
    math::v2f delta;  // Current mouse delta direction from last position
    bool hidden = false;
    bool locked = false;
};

void SetMouseLock(bool lock);
void ToggleMouseLock();
void SetMouseHide(bool hide);
void ToggleMouseHide();

math::v2i GetMouseScreenPosition();
math::v2f GetMouseDelta();

// ========================================================
// [GENERAL]
struct InputState
{
    u8 buttons[256];
    MouseState mouse;
};

void Init();
void Update();

};
};
