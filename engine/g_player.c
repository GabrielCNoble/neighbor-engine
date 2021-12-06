#include "g_player.h"
#include "phys.h"
#include "r_main.h"
#include "input.h"
#include "ent.h"

struct g_player_t g_player;
extern struct ds_list_t g_spawn_points;
extern mat4_t r_camera_matrix;

void g_PlayerInit()
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

void g_StepPlayer(float delta_time)
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

    r_SetViewPitchYaw(g_player.pitch, g_player.yaw);

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

    uint32_t left_mouse_state = in_GetMouseButtonState(SDL_BUTTON_LEFT);

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

    vec3_t from = camera_pos;
    vec3_t to = camera_pos;
    vec3_t_fmadd(&to, &to, &r_camera_matrix.rows[2].xyz, -2.3);

    if(left_mouse_state & IN_KEY_STATE_JUST_PRESSED)
    {
        float time = 0.0;
        g_player.grabbed_entity = e_Raycast(&from, &to, &time);

        if(g_player.grabbed_entity)
        {
            struct e_entity_t *entity = g_player.grabbed_entity;
            struct p_collider_t *collider = entity->collider->collider;
            vec3_t grab_point;
            mat3_t inverse_rotation;

            vec3_t_lerp(&grab_point, &from, &to, time);
            vec3_t_sub(&g_player.relative_grab_offset, &grab_point, &collider->position);
            mat3_t_transpose(&inverse_rotation, &collider->orientation);
            mat3_t_vec3_t_mul(&g_player.relative_grab_offset, &g_player.relative_grab_offset, &inverse_rotation);
            g_player.grab_time = time;
        }
    }
    else
    {
        if(!(left_mouse_state & IN_KEY_STATE_PRESSED))
        {
            g_player.grabbed_entity = NULL;
        }
        else if(g_player.grabbed_entity)
        {
            struct e_entity_t *entity = g_player.grabbed_entity;
            struct p_dynamic_collider_t *collider = (struct p_dynamic_collider_t *)entity->collider->collider;

            vec3_t cur_offset;
            vec3_t grab_point;
            vec3_t grab_offset;
            vec3_t point_on_collider;

            mat3_t_vec3_t_mul(&grab_offset, &g_player.relative_grab_offset, &collider->orientation);
            point_on_collider = grab_offset;
            vec3_t_add(&point_on_collider, &point_on_collider, &collider->position);
            vec3_t_lerp(&grab_point, &from, &to, g_player.grab_time);

            vec3_t_sub(&cur_offset, &grab_point, &collider->position);
            r_i_SetViewProjectionMatrix(NULL);
            r_i_SetModelMatrix(NULL);
            r_i_SetShader(NULL);
            r_i_SetDepth(GL_FALSE, 0);

            vec3_t_sub(&cur_offset, &cur_offset, &grab_offset);
            r_i_DrawPoint(&point_on_collider, &vec4_t_c(1.0, 1.0, 1.0, 1.0), 8.0);
            r_i_DrawLine(&point_on_collider, &grab_point, &vec4_t_c(1.0, 0.0, 0.0, 1.0), 1.0);
            r_i_SetDepth(GL_TRUE, GL_LESS);
            vec3_t_mul(&cur_offset, &cur_offset, 20.0);
            p_SetColliderVelocity(collider, &cur_offset, NULL);
//            vec3_t_mul(&cur_offset, &cur_offset, 200.0 - vec3_t_length(&collider->linear_velocity) * 100);
//            p_ApplyForce(collider, &cur_offset, &g_player.relative_grab_offset);
        }
    }
}

void g_SpawnPlayer(struct g_spawn_point_t *spawn_point)
{
//    e_
}

struct g_spawn_point_t *g_CreateSpawnPoint(vec3_t *position)
{
    uint32_t index = ds_slist_add_element(&g_spawn_points, NULL);
    struct g_spawn_point_t *spawn_point = ds_slist_get_element(&g_spawn_points, index);
//    spawn_point->index = index;
//    spawn_point->position = *position;
    return spawn_point;
}

void g_DestroySpawnPoint(struct g_spawn_point_t *spawn_point)
{

}






