#include "./input.hpp"
#include "./debug.hpp"
#include <vcruntime_string.h>

namespace ty
{
namespace input
{

InputState state = {};

bool IsKeyDown(InputKey key) 
{ 
    return state.currentKeys[key] & 0x80;
}

bool IsKeyUp(InputKey key) 
{ 
    return !(state.currentKeys[key] & 0x80);
}

bool IsKeyJustDown(InputKey key)
{
    return (state.currentKeys[key] & 0x80)
        && !(state.previousKeys[key] & 0x80);
}

bool IsKeyJustUp(InputKey key)
{
    return !(state.currentKeys[key] & 0x80)
        && (state.previousKeys[key] & 0x80);
}

math::v2i GetMouseScreenPosition() { return state.mouse.pos; }
math::v2f GetMouseDelta() { return state.mouse.delta; }
bool IsMouseLocked() { return state.mouse.locked; }
bool IsMouseHidden() { return state.mouse.hidden; }

void SetMouseLock(bool lock) { state.mouse.locked = lock; }
void ToggleMouseLock() { SetMouseLock(!state.mouse.locked); }
void SetMouseHide(bool hide)
{
    if(hide == state.mouse.hidden) return;
    state.mouse.hidden = hide;
    ShowCursor(!hide);
}
void ToggleMouseHide() { SetMouseHide(!state.mouse.hidden); }

void Init()
{
    state = {};
}

void Update()
{
    // Update keyboard button state
    memcpy(state.previousKeys, state.currentKeys, TY_KEY_COUNT * sizeof(u8));
    BOOL ret = GetKeyboardState(state.currentKeys);
    ASSERT(ret);

    // Update mouse state
    POINT cursorPoint;
    ret = GetCursorPos(&cursorPoint);
    ASSERT(ret);
    HWND activeWindow = GetActiveWindow();
    if(!activeWindow) return;   // Skip cursor updates entirely when app is minimized

    ret = ScreenToClient(activeWindow, &cursorPoint);
    ASSERT(ret);

    math::v2f currentPos  = {(f32)cursorPoint.x,        (f32)cursorPoint.y};
    math::v2f lastPos     = {(f32)state.mouse.pos.x,    (f32)state.mouse.pos.y};
    state.mouse.delta = Normalize(currentPos - lastPos);

    if(state.mouse.locked)
    {
        RECT clientRect;
        ret = GetClientRect(GetActiveWindow(), &clientRect);
        ASSERT(ret);
        POINT lockPoint = {clientRect.right / 2, clientRect.bottom / 2};
        ret = ClientToScreen(GetActiveWindow(), &lockPoint);
        ASSERT(ret);
        ret = SetCursorPos(lockPoint.x, lockPoint.y);
        ASSERT(ret);

        ret = ScreenToClient(GetActiveWindow(), &lockPoint);
        ASSERT(ret);
        cursorPoint = lockPoint;
    }
    
    state.mouse.pos = { cursorPoint.x, cursorPoint.y };
}

};
};
