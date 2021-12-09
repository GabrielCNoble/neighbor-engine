#include "r_defs.h"
#include "r_vert_defs.h"
#include "r_vert_uniform_defs.h"

void main()
{
    gl_Position = r_model_view_projection_matrix * r_position;
}
