#include "engine/core/input.hpp"
#include "engine/core/debug.hpp"

namespace ty
{
namespace input
{

InputState state = {};

bool IsKeyDown(InputKey key) 
{ 
    return state.buttons[key] & 0x80;
}

bool IsKeyUp(InputKey key) 
{ 
    return !(state.buttons[key] & 0x80);
}

bool IsKeyJustDown(InputKey key)
{
    return (state.buttons[key] & 0x80)
        && !(state.buttons[key] & 0x80);
}

bool IsKeyJustUp(InputKey key)
{
    return !(state.buttons[key] & 0x80)
        && (state.buttons[key] & 0x80);
}

math::v2i GetMouseScreenPosition() { return state.mouse.pos; }
math::v2f GetMouseDelta() { return state.mouse.delta; }

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
    BOOL ret = GetKeyboardState(state.buttons);
    ASSERT(ret);

    // Update mouse state
    POINT cursorPoint;
    ret = GetCursorPos(&cursorPoint);
    ASSERT(ret);
    ret = ScreenToClient(GetActiveWindow(), &cursorPoint);
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