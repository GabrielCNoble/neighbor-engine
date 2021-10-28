#define SDL_MAIN_HANDLED
#include "SDL2/SDL.h"
#include <stdio.h>
#include "game.h"



int main(int argc, char *argv[])
{
    if(SDL_Init(SDL_INIT_EVERYTHING) < 0)
    {
        printf("oh, shit...\n");
    }
    
    uint32_t editor_active = 0;
    if(argc > 1)
    {
        for(uint32_t arg_index = 1; arg_index < argc; arg_index++)
        {
            if(!strcmp(argv[arg_index], "-ed"))
            {
                editor_active = 1;
                break;
            }
        }
    }
    
    g_Init(editor_active);
    g_MainLoop();
    
//    r_Init();
//    p_Init();
//    a_Init();
//    g_Init();
//    in_Input();
//    
//    uint32_t editor = 0;
//    
//    
//    while(!(in_GetKeyState(SDL_SCANCODE_ESCAPE) & IN_KEY_STATE_JUST_PRESSED))
//    {
//        in_Input();
//        p_UpdateColliders();
//        g_UpdateEntities();
//        a_UpdateAnimations();
//        g_DrawEntities();
//        r_BeginFrame();
//        r_DrawBatches();
//        r_DrawImmediateBatches();
//        r_EndFrame();
//    }
}
