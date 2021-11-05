#define SDL_MAIN_HANDLED
#include "SDL2/SDL.h"
#include "main.h"

extern uint32_t g_editor;
extern uint32_t g_game_state;

int main(int argc, char *argv[])
{
    if(SDL_Init(SDL_INIT_EVERYTHING) < 0)
    {
        printf("oh, shit...\n");
    }

    uint32_t editor = 0;

    if(argc > 1)
    {
        for(uint32_t arg_index = 1; arg_index < argc; arg_index++)
        {
            if(!strcmp(argv[arg_index], "-ed"))
            {
                editor = 1;
                break;
            }
        }
    }

    r_Init();
    p_Init();
    a_Init();
    in_Input(0.016);
    l_Init();
    s_Init();
    gui_Init();
    g_Init(editor);

    while(g_game_state != G_GAME_STATE_QUIT)
    {
        float delta_time = 0.016;

        in_Input(delta_time);
        gui_BeginFrame(delta_time);

        if(in_GetKeyState(SDL_SCANCODE_ESCAPE) & IN_KEY_STATE_PRESSED)
        {
            if(g_editor)
            {
                g_SetGameState(G_GAME_STATE_EDITING);
            }
            else
            {
                g_SetGameState(G_GAME_STATE_PAUSED);
            }
        }

        switch(g_game_state)
        {
            case G_GAME_STATE_PLAYING:
                g_GameMain(delta_time);
                p_UpdateColliders(delta_time);
                a_UpdateAnimations(delta_time);
            break;

            case G_GAME_STATE_EDITING:
                ed_UpdateEditor();
            break;

            case G_GAME_STATE_MAIN_MENU:

            break;

            case G_GAME_STATE_PAUSED:

            break;
        }

        g_UpdateEntities();
        r_VisibleLights();
        r_VisibleEntitiesOnLights();
        r_VisibleEntities();
        r_BeginFrame();
        gui_EndFrame();
        r_DrawCmds();
        r_EndFrame();
    }
}
