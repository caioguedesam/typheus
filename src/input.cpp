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
    // IMPLEMENT ME
    // TODO(caio)#CONTINUE: Implement mouse input (movement tracking, button clicking)
    // Button clicking might already work?
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
