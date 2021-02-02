#include "input.h"


const Uint8 *in_keyboard;
uint8_t in_keyboard_state[SDL_NUM_SCANCODES];
uint32_t in_mouse_state[4];

int32_t in_mouse_x;
int32_t in_mouse_y;
int32_t in_mouse_dx;
int32_t in_mouse_dy;

float in_normalized_x;
float in_normalized_y;
float in_normalized_dx;
float in_normalized_dy;

uint32_t in_relative_mouse = 0;

extern uint32_t r_width;
extern uint32_t r_height;

void in_Input()
{
    SDL_PollEvent(NULL);
    in_keyboard = SDL_GetKeyboardState(NULL);
    
    uint32_t mouse_state;
    int32_t dx;
    int32_t dy;
    
    if(in_relative_mouse)
    {
        mouse_state = SDL_GetRelativeMouseState(&dx, &dy);
    }
    else
    {
        mouse_state = SDL_GetMouseState(&dx, &dy);
        
        int temp = dx;
        dx -= in_mouse_x;
        in_mouse_x = temp;
        
        temp = dy;
        dy -= in_mouse_y;
        in_mouse_y = temp;
        
        in_normalized_x = ((float)in_mouse_x / (float)r_width) * 2.0 - 1.0;
        in_normalized_y = 1.0 - ((float)in_mouse_y/ (float)r_height) * 2.0;
    }
    
    in_mouse_dx = dx;
    in_mouse_dy = dy;
    
    in_normalized_dx = (float)dx / (float)r_width;
    in_normalized_dy = -(float)dy / (float)r_height;
    
    for(uint32_t mouse_button = SDL_BUTTON_LEFT; mouse_button <= SDL_BUTTON_RIGHT; mouse_button++)
    {
        in_mouse_state[mouse_button - 1] &= ~(IN_KEY_STATE_JUST_PRESSED | IN_KEY_STATE_JUST_RELEASED);
        
        if(mouse_state & SDL_BUTTON(mouse_button))
        {
            if(!(in_mouse_state[mouse_button - 1] & IN_KEY_STATE_PRESSED))
            {
                in_mouse_state[mouse_button - 1] |= IN_KEY_STATE_JUST_PRESSED;
            }
            
            in_mouse_state[mouse_button - 1] |= IN_KEY_STATE_PRESSED;
        }
        else
        {
            if(in_mouse_state[mouse_button - 1] & IN_KEY_STATE_PRESSED)
            {
                in_mouse_state[mouse_button - 1] |= IN_KEY_STATE_JUST_RELEASED;
            }
            
            in_mouse_state[mouse_button - 1] &= ~IN_KEY_STATE_PRESSED;
        }
    }
    
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
    return in_mouse_state[button - 1];
}

void in_SetMouseRelativeMode(uint32_t enable)
{
    in_relative_mouse = enable;
    SDL_SetRelativeMouseMode(enable);
}

void in_GetMouseDelta(float *dx, float *dy)
{
    *dx = in_normalized_dx;
    *dy = in_normalized_dy; 
}

void in_GetMousePos(float *x, float *y)
{
    *x = in_normalized_x;
    *y = in_normalized_y;
}
