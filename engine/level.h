#ifndef LEVEL_H
#define LEVEL_H

#include "l_defs.h"
#include "r_main.h"

void l_Init();

void l_Shutdown();

void l_DestroyWorld();

void l_ClearLevel();

uint32_t l_DeserializeLevel(void *level_buffer, size_t buffer_size);

void l_ChangeLevel(char *level);

#endif
