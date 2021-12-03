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

    g_SetBasePath("");

    r_Init();
    p_Init();
    a_Init();
    in_Input(0.016);
    l_Init();
    s_Init();
    gui_Init();
    e_Init();
    g_Init(editor);

//    struct ed_bsp_polygon_t *polygon = ed_AllocBspPolygon(0);
//
//    struct r_vert_t vert = {};
//
//    vert.pos = vec3_t_c(-1.0, 1.0, -1.0);
//    ds_list_add_element(&polygon->vertices, &vert);
//    vert.pos = vec3_t_c(-1.0, 1.0,  1.0);
//    ds_list_add_element(&polygon->vertices, &vert);
//    vert.pos = vec3_t_c( 1.0, 1.0,  1.0);
//    ds_list_add_element(&polygon->vertices, &vert);
//    vert.pos = vec3_t_c( 1.0, 1.0, -1.0);
//    ds_list_add_element(&polygon->vertices, &vert);


    while(g_game_state != G_GAME_STATE_QUIT)
    {
        float delta_time = 0.016;

        g_UpdateDeltaTime();
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

//        struct ed_brush_t *brush = ed_GetBrush(0);
//
//        if(brush && brush->faces)
//        {
//            r_i_SetShader(NULL);
//            r_i_SetViewProjectionMatrix(NULL);
//            r_i_SetModelMatrix(NULL);
//
//            struct ed_bsp_polygon_t *polygon_copy = ed_CopyBspPolygons(polygon);
//            struct ed_bsp_polygon_t *brush_polygons = ed_BspPolygonsFromBrush(brush);
//            struct ed_bsp_polygon_t *poly = brush_polygons;
//            while(poly)
//            {
//                printf("%f %f %f -- %x\n", poly->normal.x, poly->normal.y, poly->normal.z, poly);
//                poly = poly->next;
//            }
//            printf("\n");
//
//            struct ed_bsp_node_t *bsp = ed_SolidBspFromPolygons(brush_polygons);
//
//            struct ed_bsp_polygon_t *clipped_polygons = ed_ClipPolygonToBsp(polygon_copy, bsp);
//
//            struct ed_bsp_polygon_t *clipped_polygon = clipped_polygons;
//            while(clipped_polygon)
//            {
//                for(uint32_t vert_index = 0; vert_index < clipped_polygon->vertices.cursor; vert_index++)
//                {
//                    struct r_vert_t *vert0 = ds_list_get_element(&clipped_polygon->vertices, vert_index);
//                    struct r_vert_t *vert1 = ds_list_get_element(&clipped_polygon->vertices, (vert_index + 1) % clipped_polygon->vertices.cursor);
//
//                    r_i_DrawLine(&vert0->pos, &vert1->pos, &vec4_t_c(1.0, 0.0, 0.0, 1.0), 1.0);
//                }
//                clipped_polygon = clipped_polygon->next;
//            }
//        }

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
}
