#include "input.h"


const Uint8 *in_keyboard;
uint8_t in_keyboard_state[SDL_NUM_SCANCODES];

void in_Input()
{
    SDL_PollEvent(NULL);
    in_keyboard = SDL_GetKeyboardState(NULL);
    
    for(uint32_t scancode = 0; scancode < SDL_NUM_SCANCODES; scancode++)
    {
        in_keyboard_state[scancode] &= ~(IN_KEY_STATE_JUST_PRESSED | IN_KEY_STATE_JUST_RELEASED);
        if(in_keyboard[scancode])
        {
            if(!(in_keyboard_state[scancode] & IN_KEY_STATE_PRESSED))
            {
                in_keyboard_state[scancode] |= IN_KEY_STATE_JUST_PRESSED;
            }
            
            in_keyboard_state[scancode] |= IN_KEY_STATE_PRESSED;
        }
        else
        {
            if(in_keyboard_state[scancode] & IN_KEY_STATE_PRESSED)
            {
                in_keyboard_state[scancode] |= IN_KEY_STATE_JUST_RELEASED;
            }
            
            in_keyboard_state[scancode] &= ~IN_KEY_STATE_PRESSED;
        }
    }
}

uint32_t in_GetKeyState(SDL_Scancode scancode)
{
    return in_keyboard_state[scancode];
}

uint32_t in_GetMouseButtonState(uint32_t button)
{
    return 0;
}
