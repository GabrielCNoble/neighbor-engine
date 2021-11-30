#include "g_player.h"
#include "phys.h"
#include "r_main.h"
#include "input.h"

struct g_player_state_t g_player;

void g_InitPlayer()
{
    struct p_col_def_t col_def = {};

    col_def.type = P_COLLIDER_TYPE_CHARACTER;
    col_def.character.step_height = 0.3;
    col_def.character.height = 1.7;
    col_def.character.radius = 0.3;
    col_def.character.crouch_height = 0.9;
    g_player.pitch = 0.0;
    g_player.yaw = 0.0;
    g_player.collider = (struct p_character_collider_t *)p_CreateCollider(&col_def, &vec3_t_c(0.0, 4.0, 0.0), &mat3_t_c_id());
}

void g_UpdatePlayer()
{
    float dx;
    float dy;

    in_SetMouseRelative(1);
    in_GetMouseDelta(&dx, &dy);

    g_player.pitch += dy;
    g_player.yaw -= dx;

    if(g_player.pitch > 0.5)
    {
        g_player.pitch = 0.5;
    }
    else if(g_player.pitch < -0.5)
    {
        g_player.pitch = -0.5;
    }

//    if(g_player.yaw < -1.0)
//    {
//        g_player.yaw = 0.0;
//    }
//    else if(g_player.yaw > 1.0)
//    {
//        g_player.yaw = 0.0;
//    }

    mat3_t rotation = mat3_t_c_id();
    mat3_t_rotate_y(&rotation, g_player.yaw);
    vec3_t disp = {};

    if(in_GetKeyState(SDL_SCANCODE_W) & IN_KEY_STATE_PRESSED)
    {
        disp.z = -1.0;
    }

    if(in_GetKeyState(SDL_SCANCODE_S) & IN_KEY_STATE_PRESSED)
    {
        disp.z = 1.0;
    }

    if(in_GetKeyState(SDL_SCANCODE_A) & IN_KEY_STATE_PRESSED)
    {
        disp.x = -1.0;
    }

    if(in_GetKeyState(SDL_SCANCODE_D) & IN_KEY_STATE_PRESSED)
    {
        disp.x = 1.0;
    }

    vec3_t_normalize(&disp, &disp);
    vec3_t_mul(&disp, &disp, 0.3);
    mat3_t_vec3_t_mul(&disp, &disp, &rotation);
    p_MoveCharacterCollider(g_player.collider, &disp);

    if(in_GetKeyState(SDL_SCANCODE_SPACE) & IN_KEY_STATE_JUST_PRESSED)
    {
        p_JumpCharacterCollider(g_player.collider);
    }

    vec3_t camera_pos = g_player.collider->position;
    camera_pos.y += (g_player.collider->height * 0.5 - g_player.collider->radius) - 0.2;

    r_SetViewPos(&camera_pos);
    r_SetViewPitchYaw(g_player.pitch, g_player.yaw);
}






