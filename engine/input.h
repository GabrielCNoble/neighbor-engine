#ifndef INPUT_H
#define INPUT_H

#include "../lib/SDL/include/SDL2/SDL.h"

enum IN_KEY_STATE
{
    IN_KEY_STATE_JUST_PRESSED = 1,
    IN_KEY_STATE_PRESSED = 1 << 1,
    IN_KEY_STATE_JUST_RELEASED = 1 << 2,
    IN_KEY_STATE_DOUBLE_CLICKED = 1 << 3,
    IN_KEY_STATE_DOUBLE_CLICK_FAILED = 1 << 4,
    IN_KEY_STATE_INPUT_DROPPED = 1 << 5
};

void in_Input(float delta_time);

uint32_t in_GetKeyState(SDL_Scancode scancode);

uint8_t *in_GetKeyStates();

uint32_t in_GetMouseButtonState(uint32_t button);

uint32_t *in_GetMouseButtonStates();

uint32_t in_GetMouseDoubleClickState(uint32_t button, uint32_t timeout);

void in_SetMouseRelative(uint32_t enable);

void in_SetMouseWarp(uint32_t enable);

void in_GetMouseDelta(float *dx, float *dy);

void in_GetNormalizedMousePos(float *x, float *y);

void in_GetMousePos(int32_t *x, int32_t *y);

char *in_GetTextBuffer();

void in_DropKeyboardInput();

void in_DropMouseInput();

void in_DropInput();

void in_StartTextInput();

void in_StopTextInput();




#endif // INPUT_H
