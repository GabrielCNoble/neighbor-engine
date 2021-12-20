#define SDL_MAIN_HANDLED
#include "SDL2/SDL.h"
#include "main.h"
#include "../lib/backtrace/include/backtrace.h"

extern uint32_t g_editor;
extern uint32_t g_game_state;
extern char *g_base_path;

int main(int argc, char *argv[])
{
    log_Init(argv[0], 1);
    log_LogMessage(LOG_TYPE_NOTICE, "Starting neighbor engine...");

    if(SDL_Init(SDL_INIT_EVERYTHING) < 0)
    {
        const char *error = SDL_GetError();
        log_LogMessage(LOG_TYPE_FATAL, "Couldn't initialize SDL!\nError message: %s", error);
        exit(-1);
    }

    g_base_path = argv[0];

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

//    g_SetBasePath("");

    r_Init();
    p_Init();
    a_Init();
    in_Input(0.016);
    l_Init();
    s_Init();
    gui_Init();
    e_Init();
    g_Init(editor);

    while(g_game_state != G_GAME_STATE_QUIT)
    {
        float delta_time = g_UpdateDeltaTime();
//        delta_time = 0.0166;
        in_Input(delta_time);
        gui_BeginFrame(delta_time);

        switch(g_game_state)
        {
            case G_GAME_STATE_PLAYING:
                g_GameMain(delta_time);
                a_StepAnimations(delta_time);
                p_StepPhysics(delta_time);
            break;

            case G_GAME_STATE_PAUSED:
                g_GamePaused();
            break;

            case G_GAME_STATE_MAIN_MENU:
                g_MainMenu();
            break;
        }

        e_UpdateEntities();
        r_VisibleWorld();
        r_VisibleLights();
        r_VisibleEntitiesOnLights();
        r_VisibleEntities();
        r_BeginFrame();
        gui_EndFrame();
        r_DrawCmds();
        r_EndFrame();
    }

    log_LogMessage(LOG_TYPE_NOTICE, "Shutting down neighbor engine...");

    g_Shutdown();
    e_Shutdown();
    gui_Shutdown();
    s_Shutdown();
    l_Shutdown();
    a_Shutdown();
    p_Shutdown();
    r_Shutdown();

    log_LogMessage(LOG_TYPE_NOTICE, "Neighbor engine shut down!");
    log_Shutdown();
}
