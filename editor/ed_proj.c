#include "ed_proj.h"
#include "../engine/r_draw.h"
#include "../engine/gui.h"
#include "ed_main.h"

extern uint32_t r_width;
extern uint32_t r_height;
extern struct ed_editor_t ed_editors[];

void ed_ProjEditorInit(struct ed_editor_t *editor)
{
    editor->next_state = ed_ProjEditorIdle;
}

void ed_ProjEditorShutdown()
{

}

void ed_ProjEditorSuspend()
{

}

void ed_ProjEditorResume()
{
    r_SetClearColor(0.1, 0.1, 0.1, 1.0);
}

void ed_ProjEditorUpdate()
{

}

void ed_ProjEditorIdle()
{
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar;
    window_flags |= ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoCollapse;
    igSetNextWindowPos((ImVec2){r_width / 2.0, r_height / 2.0}, 0, (ImVec2){0.5, 0.5});
    igPushStyleVar_Float(ImGuiStyleVar_WindowRounding, 8.0);

    if(igBegin("Startup", NULL, window_flags))
    {
        igText("Would you like to...");

        if(igButton("Doodle on an empty level", (ImVec2){0, 50.0}))
        {
            ed_SwitchToEditor(ed_editors + ED_EDITOR_LEVEL);
        }

        igSameLine(0.0, -1.0);

        if(igButton("Open a project folder", (ImVec2){0, 50.0}))
        {
            ed_SwitchToEditor(ed_editors + ED_EDITOR_LEVEL);
        }

        igSameLine(0.0, -1.0);

        if(igButton("Create a project folder", (ImVec2){0, 50.0}))
        {
//            ed_SetNextState(ed_SetupNewProject);
        }

        igSameLine(0.0, -1.0);

        if(igButton("Quit", (ImVec2){50.0, 50.0}))
        {
            ed_Quit();
        }
    }
    igEnd();
    igPopStyleVar(1);
}

void ed_ProjNewProject()
{

}



