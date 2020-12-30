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




#endif // INPUT_H
