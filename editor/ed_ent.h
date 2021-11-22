#ifndef ED_ENT_H
#define ED_ENT_H


#include <stdint.h>
#include "../engine/e_defs.h"


void ed_SerializeEntDef(void **buffer, size_t *buffer_size, struct e_ent_def_t *ent_def);

void ed_SaveEntDef(char *file_name, struct e_ent_def_t *ent_def);


#endif // ED_ENT_H
