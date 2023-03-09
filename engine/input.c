#include "input.h"
#include "SDL2\SDL_events.h"
#include <stdio.h>
#include <string.h>

#define IN_DOUBLE_CLICK_FRAMES 25

const Uint8 *in_keyboard;
//uint8_t in_keyboard_state[SDL_NUM_SCANCODES];
char in_text_buffer[512];
//uint32_t in_mouse_state[4];
uint32_t in_mouse_timers[4] = {IN_DOUBLE_CLICK_FRAMES, IN_DOUBLE_CLICK_FRAMES, IN_DOUBLE_CLICK_FRAMES, IN_DOUBLE_CLICK_FRAMES};

struct in_input_state_t in_input_state;

int32_t in_mouse_x;
int32_t in_mouse_y;
int32_t in_prev_mouse_x;
int32_t in_prev_mouse_y;
int32_t in_mouse_dx;
int32_t in_mouse_dy;

float in_normalized_x;
float in_normalized_y;
float in_normalized_dx;
float in_normalized_dy;

uint32_t in_relative_mouse = 0;
uint32_t in_warp_mouse = 0;
uint32_t in_warp_frame = 0;
uint32_t in_mouse_lock = 0;
uint32_t in_text_input = 0;
uint32_t in_mouse_input_dropped = 0;
uint32_t in_keybord_input_dropped = 0;

extern SDL_Window *r_window;
extern uint32_t r_width;
extern uint32_t r_height;

void in_Input(float delta_time)
{
    SDL_Event event;

    for(uint32_t mouse_button = SDL_BUTTON_LEFT; mouse_button <= SDL_BUTTON_RIGHT; mouse_button++)
    {
        uint32_t button_index = mouse_button - 1;
        in_input_state.mouse[button_index] &= ~(IN_KEY_STATE_JUST_PRESSED | IN_KEY_STATE_JUST_RELEASED | IN_KEY_STATE_DOUBLE_CLICKED);
    }

    for(uint32_t scancode = 0; scancode < SDL_NUM_SCANCODES; scancode++)
    {
        in_input_state.keyboard[scancode] &= ~(IN_KEY_STATE_JUST_PRESSED | IN_KEY_STATE_JUST_RELEASED);
    }

    in_prev_mouse_x = in_mouse_x;
    in_prev_mouse_y = in_mouse_y;

    while(SDL_PollEvent(&event))
    {
        switch(event.type)
        {
            case SDL_TEXTINPUT:
                if(in_text_input)
                {
                    strncpy(in_text_buffer, event.text.text, sizeof(in_text_buffer));
                }
            break;

            case SDL_MOUSEMOTION:
                in_mouse_x = event.motion.x;
                in_mouse_y = event.motion.y;
            break;

            case SDL_MOUSEBUTTONUP:
            case SDL_MOUSEBUTTONDOWN:
            {
                uint32_t button = event.button.button - 1;

                if(event.button.state == SDL_PRESSED)
                {
                    in_input_state.mouse[button] = IN_KEY_STATE_PRESSED | IN_KEY_STATE_JUST_PRESSED;

                    if(event.button.clicks > 1)
                    {
                        in_input_state.mouse[button] |= IN_KEY_STATE_DOUBLE_CLICKED;
                    }
                }
                else
                {
                    in_input_state.mouse[button] = IN_KEY_STATE_JUST_RELEASED;
                }
            }
            break;

            case SDL_KEYUP:
            case SDL_KEYDOWN:
                if(!event.key.repeat)
                {
                    uint32_t scancode = event.key.keysym.scancode;

                    if(event.key.state == SDL_PRESSED)
                    {
                        in_input_state.keyboard[scancode] = IN_KEY_STATE_PRESSED | IN_KEY_STATE_JUST_PRESSED;
                    }
                    else
                    {
                        in_input_state.keyboard[scancode] = IN_KEY_STATE_JUST_RELEASED;
                    }
                }
            break;
        }
    }

    if(in_relative_mouse)
    {
        SDL_GetRelativeMouseState(&in_mouse_dx, &in_mouse_dy);
    }
    else
    {
        in_mouse_dx = in_mouse_x - in_prev_mouse_x;
        in_mouse_dy = in_mouse_y - in_prev_mouse_y;

        if(in_warp_mouse)
        {
            in_warp_frame = 0;

            if(in_mouse_x < 0)
            {
                in_mouse_x = r_width + in_mouse_x;
                in_prev_mouse_x += r_width;
                in_warp_frame = 1;
            }
            else if(in_mouse_x >= r_width)
            {
                in_mouse_x = in_mouse_x - r_width;
                in_prev_mouse_x -= r_width;
                in_warp_frame = 1;
            }

            if(in_mouse_y < 0)
            {
                in_mouse_y = r_height + in_mouse_y;
                in_prev_mouse_y += r_height;
                in_warp_frame = 1;
            }
            else if(in_mouse_y >= r_height)
            {
                in_mouse_y = in_mouse_y - r_height;
                in_prev_mouse_y -= r_height;
                in_warp_frame = 1;
            }

            if(in_warp_frame)
            {
                SDL_WarpMouseInWindow(r_window, in_mouse_x, in_mouse_y);
            }
        }
    }

    in_normalized_x = ((float)in_mouse_x / (float)r_width) * 2.0 - 1.0;
    in_normalized_y = 1.0 - ((float)in_mouse_y/ (float)r_height) * 2.0;

    in_normalized_dx = (float)in_mouse_dx / (float)r_width;
    in_normalized_dy = -(float)in_mouse_dy / (float)r_height;

    in_mouse_input_dropped = 0;
    in_keybord_input_dropped = 0;
}

uint32_t in_GetKeyState(SDL_Scancode scancode)
{
    if(in_keybord_input_dropped)
    {
        return 0;
    }

    return in_input_state.keyboard[scancode];
}

uint8_t *in_GetKeyboardState()
{
    return in_input_state.keyboard;
}

uint32_t in_GetMouseButtonState(uint32_t button)
{
    if(in_mouse_input_dropped)
    {
        return 0;
    }

    return in_input_state.mouse[button - 1];
}

uint32_t *in_GetMouseButtonStates()
{
    return in_input_state.mouse;
}

uint32_t in_GetMouseDoubleClickState(uint32_t button, uint32_t timeout)
{
    uint32_t button_state = in_GetMouseButtonState(button) & ~(IN_KEY_STATE_DOUBLE_CLICKED |
                                                               IN_KEY_STATE_DOUBLE_CLICK_FAILED);

    if(!(button_state & IN_KEY_STATE_PRESSED))
    {
        if(in_mouse_timers[button - 1] < timeout)
        {
            button_state |= IN_KEY_STATE_DOUBLE_CLICKED;
        }
        else if (in_mouse_timers[button - 1] < IN_DOUBLE_CLICK_FRAMES)
        {
            button_state |= IN_KEY_STATE_DOUBLE_CLICK_FAILED;
        }
    }

    return button_state;
}

void in_SetMouseRelative(uint32_t enable)
{
    in_relative_mouse = enable;
    SDL_SetRelativeMouseMode(enable);
}

void in_SetMouseWarp(uint32_t enable)
{
    in_warp_mouse = enable;
    SDL_CaptureMouse(enable);
}

void in_SetMouseLock(uint32_t enable)
{
    in_mouse_lock = enable;
    SDL_ShowCursor(!enable);
}

void in_GetMouseDelta(float *dx, float *dy)
{
    if(in_mouse_input_dropped)
    {
        *dx = 0.0;
        *dy = 0.0;
    }
    else
    {
        *dx = in_normalized_dx;
        *dy = in_normalized_dy;
    }
}

void in_GetNormalizedMousePos(float *x, float *y)
{
    *x = in_normalized_x;
    *y = in_normalized_y;
}

void in_GetMousePos(int32_t *x, int32_t *y)
{
    *x = in_mouse_x;
    *y = in_mouse_y;
}

char *in_GetTextBuffer()
{
    return in_text_buffer;
}

void in_DropKeyboardInput()
{
    in_keybord_input_dropped = 1;
    in_text_buffer[0] = '\0';
}

void in_DropMouseInput()
{
    in_mouse_input_dropped = 1;
}

void in_DropInput()
{
    in_DropKeyboardInput();
    in_DropMouseInput();
}

void in_StartTextInput()
{
    if(!in_text_input)
    {
        in_text_input = 1;
        SDL_StartTextInput();
    }
}

void in_StopTextInput()
{
    if(in_text_input)
    {
        SDL_StopTextInput();
        in_text_input = 0;
    }
}






