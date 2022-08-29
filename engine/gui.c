#include "gui.h"
#include "r_main.h"
#include "r_draw.h"
#include "r_draw_i.h"
#include "input.h"
#include <stdint.h>
#include "log.h"

ImGuiContext *gui_context;
struct r_texture_t *gui_font_atlas;
mat4_t gui_projection_matrix;
struct r_shader_t *gui_shader;

extern uint32_t r_width;
extern uint32_t r_height;
extern struct r_framebuffer_t *r_ui_framebuffer;

struct r_vertex_layout_t gui_vertex_layout = {
    .stride = sizeof(ImDrawVert),
    .attrib_count = 3,
    .attribs = (struct r_vertex_attrib_t[]){
        {.format = R_FORMAT_RG32F,   .offset = offsetof(ImDrawVert, pos), .name = "r_position"},
        {.format = R_FORMAT_RGBA8UI, .offset = offsetof(ImDrawVert, col), .name = "r_color", .normalized = 1},
        {.format = R_FORMAT_RG32F,   .offset = offsetof(ImDrawVert, uv),  .name = "r_tex_coords"},
    }
};

void gui_Init()
{
    log_ScopedLogMessage(LOG_TYPE_NOTICE, "Initializing UI...");
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
    struct r_texture_desc_t texture_desc = {
        .width = width,
        .height = height,
        .format = R_FORMAT_RGBA8,
        .min_filter = GL_LINEAR,
        .mag_filter = GL_LINEAR,
        .addr_s = GL_CLAMP_TO_EDGE,
        .addr_t = GL_CLAMP_TO_EDGE,
    };
//    gui_font_atlas = r_CreateTexture("font_atlas", width, height, R_FORMAT_RGBA8, GL_LINEAR, GL_LINEAR, pixels);
    gui_font_atlas = r_CreateTexture("font_atlas", &texture_desc, pixels);
    ImFontAtlas_SetTexID(io->Fonts, (ImTextureID)gui_font_atlas);

    struct r_shader_desc_t shader_desc = {
        .vertex_code = "shaders/r_ui.vert",
        .fragment_code = "shaders/r_ui.frag",
        .vertex_layout = &(struct r_vertex_layout_t) {
            .stride = sizeof(ImDrawVert),
            .attrib_count = 3,
            .attribs = (struct r_vertex_attrib_t[]){
                {.format = R_FORMAT_RG32F,   .offset = offsetof(ImDrawVert, pos), .name = "r_position"},
                {.format = R_FORMAT_RGBA8UI, .offset = offsetof(ImDrawVert, col), .name = "r_color", .normalized = 1},
                {.format = R_FORMAT_RG32F,   .offset = offsetof(ImDrawVert, uv),  .name = "r_tex_coords"},
            }
        },
        .uniforms = R_DEFAULT_UNIFORMS,
        .uniform_count = sizeof(R_DEFAULT_UNIFORMS) / sizeof(R_DEFAULT_UNIFORMS[0])
    };
    gui_shader = r_LoadShader(&shader_desc);
//    gui_shader->vertex_layout = &gui_vertex_layout;
    log_ScopedLogMessage(LOG_TYPE_NOTICE, "UI initialized!");
}

void gui_Shutdown()
{
    log_ScopedLogMessage(LOG_TYPE_NOTICE, "Shutting down UI...");
    log_ScopedLogMessage(LOG_TYPE_NOTICE, "UI shut down!");
}

void gui_BeginFrame(float delta_time)
{
    ImGuiIO *io = igGetIO();
    io->DisplaySize.x = r_width;
    io->DisplaySize.y = r_height;
    int32_t mouse_x;
    int32_t mouse_y;
    uint8_t *keyboard_state = in_GetKeyStates();
    uint32_t *mouse_state = in_GetMouseButtonStates();

    in_GetMousePos(&mouse_x, &mouse_y);
    io->MousePos.x = (float)mouse_x;
    io->MousePos.y = (float)mouse_y;
    io->MouseDown[0] = mouse_state[SDL_BUTTON_LEFT - 1] & IN_KEY_STATE_PRESSED;
    io->MouseDown[1] = mouse_state[SDL_BUTTON_RIGHT - 1] & IN_KEY_STATE_PRESSED;
    io->MouseDown[2] = mouse_state[SDL_BUTTON_MIDDLE - 1] & IN_KEY_STATE_PRESSED;

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

//    igShowDemoWindow(NULL);
}

void gui_EndFrame()
{
    igRender();
    ImDrawData *draw_data = igGetDrawData();

//    r_BindFramebuffer(r_ui_framebuffer);
//    r_BindShader(gui_shader);
//    r_BindTexture(gui_font_atlas, GL_TEXTURE0);
//    r_SetDefaultUniformMat4(R_UNIFORM_MODEL_VIEW_PROJECTION_MATRIX, &gui_projection_matrix);
//    r_SetDefaultUniformI(R_UNIFORM_TEX0, 0);
//
//    glEnable(GL_BLEND);
//    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//    glEnable(GL_SCISSOR_TEST);
//    glEnable(GL_DEPTH_TEST);
//    glDepthFunc(GL_ALWAYS);
//    glDisable(GL_CULL_FACE);
//    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
//    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

//    r_i_SetViewProjectionMatrix(&gui_projection_matrix);
//    r_i_SetDepth(GL_FALSE, GL_LESS);
//    r_i_SetBlending(GL_TRUE, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//    r_i_SetModelMatrix(NULL);
//    r_i_SetRasterizer(GL_FALSE, GL_BACK, GL_FILL);
//    r_i_SetShader(gui_shader);

//    r_i_SetTexture(gui_font_atlas, GL_TEXTURE0);

//    glClearColor(0, 0, 0, 0);
//    glClear(GL_COLOR_BUFFER_BIT);

    struct r_i_blending_t blend_state = {
        .enable = GL_TRUE,
        .src_factor = GL_SRC_ALPHA,
        .dst_factor = GL_ONE_MINUS_SRC_ALPHA
    };
    r_i_SetBlending(NULL, NULL, &blend_state);

    struct r_i_depth_t depth_state = {
        .enable = GL_TRUE,
        .func = GL_ALWAYS,
    };
    r_i_SetDepth(NULL, NULL, &depth_state);

    struct r_i_raster_t raster_state = {
        .cull_enable = GL_FALSE,
        .cull_face = GL_DONT_CARE,
        .polygon_mode = GL_FILL
    };
    r_i_SetRasterizer(NULL, NULL, &raster_state);

    struct r_i_scissor_t scissor_state = {
        .enable = GL_TRUE,
    };
    r_i_SetScissor(NULL, NULL, &scissor_state);

    struct r_i_draw_mask_t draw_mask = {
        .red = GL_TRUE,
        .green = GL_TRUE,
        .blue = GL_TRUE,
        .alpha = GL_TRUE,
        .depth = GL_TRUE,
    };
    r_i_SetDrawMask(NULL, NULL, &draw_mask);

    r_i_SetShader(NULL, gui_shader);

    struct r_i_uniform_t uniforms[] = {
        (struct r_i_uniform_t){
            .uniform = R_UNIFORM_MODEL_VIEW_PROJECTION_MATRIX,
            .value = &gui_projection_matrix,
            .count = 1
        },
        (struct r_i_uniform_t){
            .uniform = R_UNIFORM_TEX0,
            .value = &(struct r_i_texture_t){
                .texture = gui_font_atlas,
                .tex_unit = 0
            },
            .count = 1
        }
    };
    r_i_SetUniforms(NULL, NULL, uniforms, 2);



//    r_i_SetUniform(NULL, R_UNIFORM_MODEL_VIEW_PROJECTION_MATRIX, &gui_projection_matrix);

//    uint32_t vert_start = R_IMMEDIATE_VERTEX_BUFFER_OFFSET / sizeof(ImDrawVert);
//    uint32_t index_start = R_IMMEDIATE_INDEX_BUFFER_OFFSET / sizeof(uint16_t);

    for(uint32_t list_index = 0; list_index < draw_data->CmdListsCount; list_index++)
    {
        ImDrawList *src_list = draw_data->CmdLists[list_index];
        struct r_i_draw_list_t *dst_list = r_i_AllocDrawList(NULL, src_list->CmdBuffer.Size);

        struct r_i_mesh_t *mesh = r_i_AllocMesh(NULL, sizeof(ImDrawVert), src_list->VtxBuffer.Size, src_list->IdxBuffer.Size);

        for(uint32_t index = 0; index < src_list->IdxBuffer.Size; index++)
        {
            mesh->indices.indices[index] = src_list->IdxBuffer.Data[index];
        }

        memcpy(mesh->verts.verts, src_list->VtxBuffer.Data, sizeof(ImDrawVert) * src_list->VtxBuffer.Size);

        dst_list->mesh = mesh;
        dst_list->mode = GL_TRIANGLES;
        dst_list->indexed = 1;

        for(uint32_t cmd_index = 0; cmd_index < src_list->CmdBuffer.Size; cmd_index++)
        {
            ImDrawCmd *src_cmd = src_list->CmdBuffer.Data + cmd_index;
            struct r_i_draw_range_t *range = dst_list->ranges + cmd_index;

            uint32_t clip_x = src_cmd->ClipRect.x;
            uint32_t clip_y = (float)r_height - src_cmd->ClipRect.w;
            uint32_t clip_w = src_cmd->ClipRect.z - src_cmd->ClipRect.x;
            uint32_t clip_h = src_cmd->ClipRect.w - src_cmd->ClipRect.y;

            struct r_i_scissor_t scissor_state = {
                .x = clip_x,
                .y = clip_y,
                .width = clip_w,
                .height = clip_h,
                .enable = GL_DONT_CARE
            };

            r_i_SetScissor(NULL, range, &scissor_state);

            range->start = src_cmd->IdxOffset;
            range->count = src_cmd->ElemCount;
        }

        r_i_DrawList(NULL, dst_list);
    }

//    glDisable(GL_SCISSOR_TEST);
//    glDisable(GL_BLEND);
//    glEnable(GL_DEPTH_TEST);
//    glDepthFunc(GL_LESS);
}




















