#include "./input.hpp"
#include "./debug.hpp"
#include <vcruntime_string.h>

namespace ty
{
namespace input
{

Context MakeInputContext()
{
    Context ctx = {};
    return ctx;
}

bool IsKeyDown(Context* ctx, InputKey key) 
{ 
    ASSERT(ctx);
    return ctx->currentKeys[key] & 0x80;
}

bool IsKeyUp(Context* ctx, InputKey key) 
{ 
    ASSERT(ctx);
    return !(ctx->currentKeys[key] & 0x80);
}

bool IsKeyJustDown(Context* ctx, InputKey key)
{
    ASSERT(ctx);
    return  (ctx->currentKeys[key] & 0x80)
        && !(ctx->previousKeys[key] & 0x80);
}

bool IsKeyJustUp(Context* ctx, InputKey key)
{
    ASSERT(ctx);
    return !(ctx->currentKeys[key] & 0x80)
        &&  (ctx->previousKeys[key] & 0x80);
}

math::v2i GetMouseScreenPosition(Context* ctx)
{
    ASSERT(ctx);
    return ctx->mouse.pos; 
}

math::v2f GetMouseDelta(Context* ctx) 
{
    return ctx->mouse.delta; 
}

bool IsMouseLocked(Context* ctx) 
{ 
    ASSERT(ctx);
    return ctx->mouse.locked; 
}

bool IsMouseHidden(Context* ctx) 
{ 
    ASSERT(ctx);
    return ctx->mouse.hidden; 
}

void SetMouseLock(Context* ctx, bool lock) 
{ 
    ASSERT(ctx);
    ctx->mouse.locked = lock; 
}
void ToggleMouseLock(Context* ctx) 
{ 
    SetMouseLock(ctx, !ctx->mouse.locked); 
}
void SetMouseHide(Context* ctx, bool hide)
{
    ASSERT(ctx);
    if(hide == ctx->mouse.hidden) return;
    ctx->mouse.hidden = hide;
    ShowCursor(!hide);
}
void ToggleMouseHide(Context* ctx)
{
    ASSERT(ctx);
    SetMouseHide(ctx, !ctx->mouse.hidden); 
}

void UpdateInput(Context* ctx)
{
    ASSERT(ctx);
    // Update keyboard button state
    memcpy(ctx->previousKeys, ctx->currentKeys, TY_KEY_COUNT * sizeof(u8));
    BOOL ret = GetKeyboardState(ctx->currentKeys);
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
    math::v2f lastPos     = {(f32)ctx->mouse.pos.x,    (f32)ctx->mouse.pos.y};
    ctx->mouse.delta = Normalize(currentPos - lastPos);

    if(ctx->mouse.locked)
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
    
    ctx->mouse.pos = { cursorPoint.x, cursorPoint.y };
}

};
};
