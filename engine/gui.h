#ifndef GUI_H
#define GUI_H

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "../lib/cimgui/cimgui.h"

void gui_Init();

void gui_Shutdown();

void gui_BeginFrame(float delta_time);

void gui_EndFrame();


#endif // GUI_H
