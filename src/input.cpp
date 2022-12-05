// TYPHEUS ENGINE - INPUT HANDLING
#include "input.hpp"

InputState currentState;
InputState lastState;

void UpdateInputState()
{
    lastState = currentState;

    BOOL ret = GetKeyboardState(currentState.buttons);
    ASSERT(ret);
    
    UpdateMouseState(&currentState.mouse);
}

void UpdateMouseState(MouseState* mouseState)
{
    POINT cursorPoint;
    BOOL ret = GetCursorPos(&cursorPoint);
    ASSERT(ret);
    ret = ScreenToClient(GetActiveWindow(), &cursorPoint);
    
    currentState.mouse.pos =
    {
        cursorPoint.x, cursorPoint.y
    };
}

bool IsKeyDown(InputKey key)
{
    return currentState.buttons[key] & 0x80;
}

bool IsKeyUp(InputKey key)
{
    return !(currentState.buttons[key] & 0x80);
}

bool IsKeyJustDown(InputKey key)
{
    return (currentState.buttons[key] & 0x80)
        && !(lastState.buttons[key] & 0x80);
}

bool IsKeyJustUp(InputKey key)
{
    return !(currentState.buttons[key] & 0x80)
        && (lastState.buttons[key] & 0x80);
}

v2i GetMousePosition()
{
    return currentState.mouse.pos;
}

v2f GetMouseDelta()
{
    v2f currentPos = {(f32)currentState.mouse.pos.x, (f32)currentState.mouse.pos.y};
    v2f lastPos = {(f32)lastState.mouse.pos.x, (f32)lastState.mouse.pos.y};
    return Normalize(currentPos - lastPos);
}
