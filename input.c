#include "input.h"
#include <stdio.h>

#define IN_DOUBLE_CLICK_FRAMES 25

const Uint8 *in_keyboard;
uint8_t in_keyboard_state[SDL_NUM_SCANCODES];
char in_text_buffer[512];
uint32_t in_mouse_state[4];
uint32_t in_mouse_timers[4] = {IN_DOUBLE_CLICK_FRAMES, IN_DOUBLE_CLICK_FRAMES, IN_DOUBLE_CLICK_FRAMES, IN_DOUBLE_CLICK_FRAMES};

int32_t in_mouse_x;
int32_t in_mouse_y;
int32_t in_mouse_dx;
int32_t in_mouse_dy;

float in_normalized_x;
float in_normalized_y;
float in_normalized_dx;
float in_normalized_dy;

uint32_t in_relative_mouse = 0;
uint32_t in_text_input = 0;

extern uint32_t r_width;
extern uint32_t r_height;

void in_Input(float delta_time)
{
    SDL_Event event;
    uint32_t has_event = SDL_PollEvent(&event);

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
        uint32_t button_index = mouse_button - 1;
        in_mouse_state[button_index] &= ~(IN_KEY_STATE_JUST_PRESSED |
                                          IN_KEY_STATE_JUST_RELEASED |
                                          IN_KEY_STATE_DOUBLE_CLICKED |
                                          IN_KEY_STATE_DOUBLE_CLICK_FAILED);

        if(mouse_state & SDL_BUTTON(mouse_button))
        {
            if(!(in_mouse_state[button_index] & IN_KEY_STATE_PRESSED))
            {
                in_mouse_state[button_index] |= IN_KEY_STATE_JUST_PRESSED;

                if(in_mouse_timers[button_index] == IN_DOUBLE_CLICK_FRAMES)
                {
                    in_mouse_timers[button_index] = 0;
                }
            }

            if(in_mouse_timers[button_index] && in_mouse_timers[button_index] < IN_DOUBLE_CLICK_FRAMES)
            {
                in_mouse_state[button_index] |= IN_KEY_STATE_DOUBLE_CLICKED;
                in_mouse_timers[button_index] = IN_DOUBLE_CLICK_FRAMES;
            }

            in_mouse_state[button_index] |= IN_KEY_STATE_PRESSED;
        }
        else
        {
            if(in_mouse_state[button_index] & IN_KEY_STATE_PRESSED)
            {
                in_mouse_state[button_index] |= IN_KEY_STATE_JUST_RELEASED;
            }

            in_mouse_state[button_index] &= ~IN_KEY_STATE_PRESSED;

            if(in_mouse_timers[button_index] < IN_DOUBLE_CLICK_FRAMES)
            {
                in_mouse_timers[button_index]++;

                if(in_mouse_timers[button_index] == IN_DOUBLE_CLICK_FRAMES)
                {
                    in_mouse_state[button_index] |= IN_KEY_STATE_DOUBLE_CLICK_FAILED;
                }
            }
        }
    }

    in_text_buffer[0] = '\0';
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

    if(in_text_input)
    {
        while(has_event)
        {
            if(event.type == SDL_TEXTINPUT)
            {
                strncpy(in_text_buffer, event.text.text, sizeof(in_text_buffer));
            }

            has_event = SDL_PollEvent(&event);
        }
    }
}

uint32_t in_GetKeyState(SDL_Scancode scancode)
{
    return in_keyboard_state[scancode];
}

uint8_t *in_GetKeyStates()
{
    return in_keyboard_state;
}

uint32_t in_GetMouseButtonState(uint32_t button)
{
    return in_mouse_state[button - 1];
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
    for(uint32_t scancode = 0; scancode < SDL_NUM_SCANCODES; scancode++)
    {
        in_keyboard_state[scancode] = 0;
    }

    in_text_buffer[0] = '\0';
}

void in_DropMouseInput()
{
    for(uint32_t mouse_button = SDL_BUTTON_LEFT; mouse_button <= SDL_BUTTON_RIGHT; mouse_button++)
    {
        in_mouse_state[mouse_button - 1] = 0;
    }

    in_mouse_dx = 0;
    in_mouse_dy = 0;
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






