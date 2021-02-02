#ifndef INPUT_H
#define INPUT_H

#include "SDL2/SDL.h"

enum IN_KEY_STATE
{
    IN_KEY_STATE_JUST_PRESSED = 1,
    IN_KEY_STATE_PRESSED = 1 << 1,
    IN_KEY_STATE_JUST_RELEASED = 1 << 2
};

void in_Input();

uint32_t in_GetKeyState(SDL_Scancode scancode);

uint32_t in_GetMouseButtonState(uint32_t button);

void in_SetMouseRelativeMode(uint32_t enable);

void in_GetMouseDelta(float *dx, float *dy);

void in_GetMousePos(float *x, float *y);




#endif // INPUT_H
