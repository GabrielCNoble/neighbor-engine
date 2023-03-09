#ifndef ED_ENT_H
#define ED_ENT_H

#include "obj.h"
#include "../engine/e_defs.h"

struct ed_ent_args_t
{
    struct e_ent_def_t *def;
};

void ed_InitEntityObjectFuncs();

#endif // ED_ENT_H
