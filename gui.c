#include "gui.h"
#include "r_main.h"
#include "r_draw.h"
#include "input.h"
#include <stdint.h>

ImGuiContext *gui_context;
struct r_texture_t *gui_font_atlas;
mat4_t gui_projection_matrix;
struct r_shader_t *gui_shader;

extern uint32_t r_width;
extern uint32_t r_height;

void gui_Init()
{
    gui_context = igCreateContext(NULL);
    igSetCurrentContext(gui_context);

    ImGuiIO *io = igGetIO();

    io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io->IniFilename = NULL;

    io->KeyMap[ImGuiKey_Tab] = SDL_SCANCODE_TAB;
    io->KeyMap[ImGuiKey_LeftArrow] = SDL_SCANCODE_LEFT;
    io->KeyMap[ImGuiKey_RightArrow] = SDL_SCANCODE_RIGHT;
    io->KeyMap[ImGuiKey_UpArrow] = SDL_SCANCODE_UP;
    io->KeyMap[ImGuiKey_DownArrow] = SDL_SCANCODE_DOWN;
    io->KeyMap[ImGuiKey_PageUp] = SDL_SCANCODE_PAGEUP;
    io->KeyMap[ImGuiKey_PageDown] = SDL_SCANCODE_PAGEDOWN;
    io->KeyMap[ImGuiKey_Home] = SDL_SCANCODE_HOME;
    io->KeyMap[ImGuiKey_End] = SDL_SCANCODE_END;
    io->KeyMap[ImGuiKey_Insert] = SDL_SCANCODE_INSERT;
    io->KeyMap[ImGuiKey_Delete] = SDL_SCANCODE_DELETE;
    io->KeyMap[ImGuiKey_Backspace] = SDL_SCANCODE_BACKSPACE;
    io->KeyMap[ImGuiKey_Space] = SDL_SCANCODE_SPACE;
    io->KeyMap[ImGuiKey_Enter] = SDL_SCANCODE_RETURN;
    io->KeyMap[ImGuiKey_Escape] = SDL_SCANCODE_ESCAPE;
    io->KeyMap[ImGuiKey_KeyPadEnter] = SDL_SCANCODE_KP_ENTER;
    io->KeyMap[ImGuiKey_A] = SDL_SCANCODE_A;
    io->KeyMap[ImGuiKey_C] = SDL_SCANCODE_C;
    io->KeyMap[ImGuiKey_V] = SDL_SCANCODE_V;
    io->KeyMap[ImGuiKey_X] = SDL_SCANCODE_X;
    io->KeyMap[ImGuiKey_Y] = SDL_SCANCODE_Y;
    io->KeyMap[ImGuiKey_Z] = SDL_SCANCODE_Z;

    unsigned char *pixels;
    int width;
    int height;
    ImFontAtlas_GetTexDataAsRGBA32(io->Fonts, &pixels, &width, &height, NULL);
    gui_font_atlas = r_CreateTexture("font_atlas", width, height, GL_RGBA8, pixels);
    ImFontAtlas_SetTexID(io->Fonts, (ImTextureID)gui_font_atlas);

    gui_shader = r_LoadShader("shaders/r_ui.vert", "shaders/r_ui.frag");
}

void gui_Shutdown()
{

}

void gui_BeginFrame()
{
    ImGuiIO *io = igGetIO();
    io->DisplaySize.x = r_width;
    io->DisplaySize.y = r_height;
    int32_t mouse_x;
    int32_t mouse_y;
    uint8_t *keyboard_state = in_GetKeyStates();

    in_GetMousePos(&mouse_x, &mouse_y);
    io->MousePos.x = (float)mouse_x;
    io->MousePos.y = (float)mouse_y;
    io->MouseDown[0] = in_GetMouseButtonState(SDL_BUTTON_LEFT) & IN_KEY_STATE_PRESSED;

    if(io->WantTextInput)
    {
        in_StartTextInput();
        ImGuiIO_AddInputCharactersUTF8(io, in_GetTextBuffer());
    }
    else
    {
        in_StopTextInput();
    }

    if(io->WantCaptureKeyboard)
    {
        for(uint32_t scancode = 0; scancode < SDL_NUM_SCANCODES; scancode++)
        {
            io->KeysDown[scancode] = (keyboard_state[scancode] & IN_KEY_STATE_PRESSED) && 1;
        }

        in_DropKeyboardInput();
    }

    if(io->WantCaptureMouse)
    {
        in_DropMouseInput();
    }

    mat4_t_ortho(&gui_projection_matrix, r_width, r_height, 0.0, 1.0);
    gui_projection_matrix.rows[3].x = -1.0;
    gui_projection_matrix.rows[3].y = 1.0;

    igNewFrame();
//    igSetNextWindowPos((ImVec2){0.0, 0.0}, ImGuiCond_Once, (ImVec2){0.0, 0.0});
//    if(igBegin("blah", NULL, 0))
//    {
//        igText("BLAH");
//    }
//    igEnd();
//    igShowDemoWindow(NULL);
}

void gui_EndFrame()
{
    igRender();
    ImDrawData *draw_data = igGetDrawData();

    r_i_SetViewProjectionMatrix(&gui_projection_matrix);
    r_i_SetDepth(GL_FALSE, GL_LESS);
    r_i_SetBlending(GL_TRUE, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    r_i_SetModelMatrix(NULL);
    r_i_SetRasterizer(GL_FALSE, GL_BACK, GL_FILL);
    r_i_SetShader(gui_shader);

    r_i_SetTexture(gui_font_atlas, GL_TEXTURE0);

    for(uint32_t list_index = 0; list_index < draw_data->CmdListsCount; list_index++)
    {
        ImDrawList *src_list = draw_data->CmdLists[list_index];

        struct r_i_verts_t *verts = r_i_AllocImmediateData(sizeof(struct r_i_verts_t) + sizeof(struct r_vert_t) * src_list->VtxBuffer.Size);
        verts->count = src_list->VtxBuffer.Size;
        struct r_i_indices_t *indices = r_i_AllocImmediateData(sizeof(struct r_i_indices_t) + sizeof(uint32_t) * src_list->IdxBuffer.Size);
        indices->count = src_list->IdxBuffer.Size;

        for(uint32_t vert_index = 0; vert_index < src_list->VtxBuffer.Size; vert_index++)
        {
            ImDrawVert *draw_vert = src_list->VtxBuffer.Data + vert_index;
            struct r_vert_t *vert = verts->verts + vert_index;

            vert->pos.x = draw_vert->pos.x;
            vert->pos.y = draw_vert->pos.y;
            vert->pos.z = 0.0;

            vert->color.x = (float)(draw_vert->col & 0xff) / 255.0;
            vert->color.y = (float)((draw_vert->col >> 8) & 0xff) / 255.0;
            vert->color.z = (float)((draw_vert->col >> 16) & 0xff) / 255.0;
            vert->color.w = (float)((draw_vert->col >> 24) & 0xff) / 255.0;

            vert->tex_coords.x = draw_vert->uv.x;
            vert->tex_coords.y = draw_vert->uv.y;
        }

        for(uint32_t index = 0; index < src_list->IdxBuffer.Size; index++)
        {
            indices->indices[index] = src_list->IdxBuffer.Data[index];
        }

        r_i_SetBuffers(verts, indices);

        for(uint32_t cmd_index = 0; cmd_index < src_list->CmdBuffer.Size; cmd_index++)
        {
            struct r_i_draw_list_t *dst_list = r_i_AllocImmediateData(sizeof(struct r_i_draw_list_t));
            dst_list->command_count = src_list->CmdBuffer.Size;
            dst_list->commands = r_i_AllocImmediateData(sizeof(struct r_i_cmd_t) * dst_list->command_count);
            dst_list->indexed = 1;

            ImDrawCmd *src_cmd = src_list->CmdBuffer.Data + cmd_index;
            struct r_i_draw_cmd_t *dst_cmd = dst_list->commands + cmd_index;

            uint32_t clip_x = src_cmd->ClipRect.x;
            uint32_t clip_y = (float)r_height - src_cmd->ClipRect.w;
            uint32_t clip_w = src_cmd->ClipRect.z - src_cmd->ClipRect.x;
            uint32_t clip_h = src_cmd->ClipRect.w - src_cmd->ClipRect.y;
            r_i_SetScissor(GL_TRUE, clip_x, clip_y, clip_w, clip_h);

            dst_cmd->start = src_cmd->IdxOffset;
            dst_cmd->count = src_cmd->ElemCount;
            r_i_DrawImmediate(R_I_DRAW_CMD_TRIANGLE_LIST, dst_list);
        }
    }
}




















