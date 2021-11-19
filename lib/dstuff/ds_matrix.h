#ifndef MATRIX_H
#define MATRIX_H

#include "ds_vector.h"
#include <intrin.h>

typedef struct
{
    union
    {
        float comps[2][2];

        struct
        {
            float x0, y0;
            float x1, y1;
        };

        vec2_t rows[2];
    };
}mat2_t;



typedef struct
{
    union
    {
        float comps[3][3];

        struct
        {
            float x0, y0, z0;
            float x1, y1, z1;
            float x2, y2, z2;
        };

        vec3_t rows[3];
    };

}mat3_t;

#ifdef __cplusplus
extern "C"
{
#endif

#define mat3_t_c_id() ((mat3_t){1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0});

void mat3_t_mul(mat3_t *r, mat3_t *a, mat3_t *b);

void mat3_t_identity(mat3_t *m);

void mat3_t_transpose(mat3_t *r, mat3_t *m);

void mat3_t_rotate_x(mat3_t *m, float angle);

void mat3_t_rotate_y(mat3_t *m, float angle);

void mat3_t_rotate_z(mat3_t *m, float angle);

void mat3_t_vec3_t_mul(vec3_t *r, vec3_t *v, mat3_t *m);

#ifdef __cplusplus
}
#endif

/*
=====================================================================
=====================================================================
=====================================================================
*/

typedef struct mat4_t
{
    union
    {
        float comps[4][4];

        struct
        {
            float x0, y0, z0, w0;
            float x1, y1, z1, w1;
            float x2, y2, z2, w2;
            float x3, y3, z3, w3;
        };

        vec4_t rows[4];
    };

}mat4_t;

#ifdef __cplusplus
extern "C"
{
#endif

void mat4_t_comp(mat4_t *r, mat3_t *rot, vec3_t *pos);

void mat4_t_mul(mat4_t *r, mat4_t *a, mat4_t *b);

void mat4_t_identity(mat4_t *m);

void mat4_t_transpose(mat4_t *r, mat4_t *m);

void mat4_t_rotate_x(mat4_t *m, float angle);

void mat4_t_rotate_y(mat4_t *m, float angle);

void mat4_t_rotate_z(mat4_t *m, float angle);

void mat4_t_invvm(mat4_t *r, mat4_t *m);

void mat4_t_persp(mat4_t* m, float fov_y, float aspect, float z_near, float z_far);

void mat4_t_gl_reverse_z_persp(mat4_t* m, float fov_y, float aspect, float z_near, float z_far);

void mat4_t_ortho(mat4_t *m, float width, float height, float z_near, float z_far);

void mat4_t_ortho_reverse_z(mat4_t *m, float width, float height, float z_near, float z_far);

void mat4_t_pitch(mat4_t* m, float pitch);

void mat4_t_yaw(mat4_t* m, float yaw);

void mat4_t_vec4_t_mul(vec4_t *r, mat4_t *m, vec4_t *v);

void mat4_t_vec4_t_mul_fast(vec4_t *r, mat4_t *m, vec4_t *v);

#ifdef __cplusplus
}
#endif

/*
=====================================================================
=====================================================================
=====================================================================
*/

#ifdef DS_MATRIX_IMPLEMENTATION

#ifdef __cplusplus
extern "C"
{
#endif

void mat4_t_comp(mat4_t *r, mat3_t *rot, vec3_t *pos)
{
    *r = (mat4_t)
    {{{
        {rot->comps[0][0], rot->comps[0][1], rot->comps[0][2], 0.0},
        {rot->comps[1][0], rot->comps[1][1], rot->comps[1][2], 0.0},
        {rot->comps[2][0], rot->comps[2][1], rot->comps[2][2], 0.0},
        {pos->comps[0],    pos->comps[1],    pos->comps[2],    1.0}
    }}};
}

void mat3_t_mul(mat3_t *r, mat3_t *a, mat3_t *b)
{
    *r = (mat3_t)
    {{{
        {a->rows[0].x * b->rows[0].x + a->rows[0].y * b->rows[1].x + a->rows[0].z * b->rows[2].x,
        a->rows[0].x * b->rows[0].y + a->rows[0].y * b->rows[1].y + a->rows[0].z * b->rows[2].y,
        a->rows[0].x * b->rows[0].z + a->rows[0].y * b->rows[1].z + a->rows[0].z * b->rows[2].z},

        {a->rows[1].x * b->rows[0].x + a->rows[1].y * b->rows[1].x + a->rows[1].z * b->rows[2].x,
        a->rows[1].x * b->rows[0].y + a->rows[1].y * b->rows[1].y + a->rows[1].z * b->rows[2].y,
        a->rows[1].x * b->rows[0].z + a->rows[1].y * b->rows[1].z + a->rows[1].z * b->rows[2].z},

        {a->rows[2].x * b->rows[0].x + a->rows[2].y * b->rows[1].x + a->rows[2].z * b->rows[2].x,
        a->rows[2].x * b->rows[0].y + a->rows[2].y * b->rows[1].y + a->rows[2].z * b->rows[2].y,
        a->rows[2].x * b->rows[0].z + a->rows[2].y * b->rows[1].z + a->rows[2].z * b->rows[2].z},
    }}};
}

void mat3_t_identity(mat3_t *m)
{
    *m = (mat3_t)
    {{{
        {1.0, 0.0, 0.0},
        {0.0, 1.0, 0.0},
        {0.0, 0.0, 1.0}
    }}};
}

void mat3_t_transpose(mat3_t *r, mat3_t *m)
{
    *r = (mat3_t)
    {{{
        {m->comps[0][0], m->comps[1][0], m->comps[2][0]},
        {m->comps[0][1], m->comps[1][1], m->comps[2][1]},
        {m->comps[0][2], m->comps[1][2], m->comps[2][2]},
    }}};
}

void mat3_t_rotate_x(mat3_t *m, float angle)
{
    float s;
    float c;

    s = sin(angle * 3.14159265);
    c = cos(angle * 3.14159265);

    mat3_t_mul(m, m, &(mat3_t){{{ {1.0, 0.0, 0.0}, {0.0, c, s}, {0.0, -s, c} }}});
}

void mat3_t_rotate_y(mat3_t *m, float angle)
{
    float s;
    float c;

    s = sin(angle * 3.14159265);
    c = cos(angle * 3.14159265);

    mat3_t_mul(m, m, &(mat3_t){{{ {c, 0.0, -s}, {0.0, 1.0, 0.0}, {s, 0.0, c} }}});
}

void mat3_t_rotate_z(mat3_t *m, float angle)
{
    float s;
    float c;

    s = sin(angle * 3.14159265);
    c = cos(angle * 3.14159265);

    mat3_t_mul(m, m, &(mat3_t){{{ {c, s, 0.0}, {-s, c, 0.0}, {0.0, 0.0, 1.0} }}});
}

extern void mat3_t_vec3_t_mul(vec3_t *r, vec3_t *v, mat3_t *m)
{
    vec3_t t;

    t.x = v->x * m->rows[0].x;
    t.y = v->x * m->rows[0].y;
    t.z = v->x * m->rows[0].z;

    t.x += v->y * m->rows[1].x;
    t.y += v->y * m->rows[1].y;
    t.z += v->y * m->rows[1].z;

    t.x += v->z * m->rows[2].x;
    t.y += v->z * m->rows[2].y;
    t.z += v->z * m->rows[2].z;

    *r = t;
}

void mat4_t_mul(mat4_t *r, mat4_t *a, mat4_t *b)
{
    *r = (mat4_t)
    {{{
        {a->rows[0].x * b->rows[0].x + a->rows[0].y * b->rows[1].x + a->rows[0].z * b->rows[2].x + a->rows[0].w * b->rows[3].x,
        a->rows[0].x * b->rows[0].y + a->rows[0].y * b->rows[1].y + a->rows[0].z * b->rows[2].y + a->rows[0].w * b->rows[3].y,
        a->rows[0].x * b->rows[0].z + a->rows[0].y * b->rows[1].z + a->rows[0].z * b->rows[2].z + a->rows[0].w * b->rows[3].z,
        a->rows[0].x * b->rows[0].w + a->rows[0].y * b->rows[1].w + a->rows[0].z * b->rows[2].w + a->rows[0].w * b->rows[3].w},

        {a->rows[1].x * b->rows[0].x + a->rows[1].y * b->rows[1].x + a->rows[1].z * b->rows[2].x + a->rows[1].w * b->rows[3].x,
        a->rows[1].x * b->rows[0].y + a->rows[1].y * b->rows[1].y + a->rows[1].z * b->rows[2].y + a->rows[1].w * b->rows[3].y,
        a->rows[1].x * b->rows[0].z + a->rows[1].y * b->rows[1].z + a->rows[1].z * b->rows[2].z + a->rows[1].w * b->rows[3].z,
        a->rows[1].x * b->rows[0].w + a->rows[1].y * b->rows[1].w + a->rows[1].z * b->rows[2].w + a->rows[1].w * b->rows[3].w},

        {a->rows[2].x * b->rows[0].x + a->rows[2].y * b->rows[1].x + a->rows[2].z * b->rows[2].x + a->rows[2].w * b->rows[3].x,
        a->rows[2].x * b->rows[0].y + a->rows[2].y * b->rows[1].y + a->rows[2].z * b->rows[2].y + a->rows[2].w * b->rows[3].y,
        a->rows[2].x * b->rows[0].z + a->rows[2].y * b->rows[1].z + a->rows[2].z * b->rows[2].z + a->rows[2].w * b->rows[3].z,
        a->rows[2].x * b->rows[0].w + a->rows[2].y * b->rows[1].w + a->rows[2].z * b->rows[2].w + a->rows[2].w * b->rows[3].w},

        {a->rows[3].x * b->rows[0].x + a->rows[3].y * b->rows[1].x + a->rows[3].z * b->rows[2].x + a->rows[3].w * b->rows[3].x,
        a->rows[3].x * b->rows[0].y + a->rows[3].y * b->rows[1].y + a->rows[3].z * b->rows[2].y + a->rows[3].w * b->rows[3].y,
        a->rows[3].x * b->rows[0].z + a->rows[3].y * b->rows[1].z + a->rows[3].z * b->rows[2].z + a->rows[3].w * b->rows[3].z,
        a->rows[3].x * b->rows[0].w + a->rows[3].y * b->rows[1].w + a->rows[3].z * b->rows[2].w + a->rows[3].w * b->rows[3].w},
    }}};
}

void mat4_t_identity(mat4_t *m)
{
    *m = (mat4_t)
    {{{
        {1.0, 0.0, 0.0, 0.0},
        {0.0, 1.0, 0.0, 0.0},
        {0.0, 0.0, 1.0, 0.0},
        {0.0, 0.0, 0.0, 1.0},
    }}};
}

void mat4_t_transpose(mat4_t *r, mat4_t *m)
{
    *r = (mat4_t)
    {{{
        {m->comps[0][0], m->comps[1][0], m->comps[2][0], m->comps[3][0]},
        {m->comps[0][1], m->comps[1][1], m->comps[2][1], m->comps[3][1]},
        {m->comps[0][2], m->comps[1][2], m->comps[2][2], m->comps[3][2]},
        {m->comps[0][3], m->comps[1][3], m->comps[2][3], m->comps[3][3]},
    }}};
}

void mat4_t_rotate_x(mat4_t *m, float angle)
{
    float s;
    float c;

    s = sin(angle * 3.14159265);
    c = cos(angle * 3.14159265);

    mat4_t_mul(m, m, &(mat4_t){{{ {1.0, 0.0, 0.0, 0.0}, {0.0, c, s, 0.0}, {0.0, -s, c, 0.0}, {0.0, 0.0, 0.0, 1.0} }}});
}

void mat4_t_rotate_y(mat4_t *m, float angle)
{
    float s;
    float c;

    s = sin(angle * 3.14159265);
    c = cos(angle * 3.14159265);

    mat4_t_mul(m, m, &(mat4_t){{{ {c, 0.0, -s, 0.0}, {0.0, 1.0, 0.0, 0.0}, {s, 0.0, c, 0.0}, {0.0, 0.0, 0.0, 1.0} }}});
}

void mat4_t_rotate_z(mat4_t *m, float angle)
{
    float s;
    float c;

    s = sin(angle * 3.14159265);
    c = cos(angle * 3.14159265);

    mat4_t_mul(m, m, &(mat4_t){{{ {c, s, 0.0, 0.0}, {-s, c, 0.0, 0.0}, {0.0, 0.0, 1.0, 0.0}, {0.0, 0.0, 0.0, 1.0} }}});
}

void mat4_t_invvm(mat4_t *r, mat4_t *m)
{
    mat4_t_transpose(r, m);

    r->rows[3] = (vec4_t)
    {{{
        -r->comps[0][3] * r->comps[0][0] - r->comps[1][3] * r->comps[1][0] - r->comps[2][3] * r->comps[2][0],
        -r->comps[0][3] * r->comps[0][1] - r->comps[1][3] * r->comps[1][1] - r->comps[2][3] * r->comps[2][1],
        -r->comps[0][3] * r->comps[0][2] - r->comps[1][3] * r->comps[1][2] - r->comps[2][3] * r->comps[2][2],
        1.0
    }}};

    r->comps[0][3] = 0.0;
    r->comps[1][3] = 0.0;
    r->comps[2][3] = 0.0;
}

void mat4_t_persp(mat4_t* m, float fov_y, float aspect, float z_near, float z_far)
{
    float t = tanf(fov_y) * z_near;
    float r = t * aspect;

    mat4_t_identity(m);

    m->comps[0][0] = z_near / r;
    m->comps[1][1] = z_near / t;
    m->comps[2][2] = (-z_far + z_near) / (z_far - z_near);
    m->comps[2][3] = -1.0;
    m->comps[3][2] = -(2.0 * z_near * z_far) / (z_far - z_near);
    m->comps[3][3] = 0.0;
}

void mat4_t_gl_reverse_z_persp(mat4_t* m, float fov_y, float aspect, float z_near, float z_far)
{
    float t = tanf(fov_y) * z_near;
    float r = t * aspect;

    mat4_t_identity(m);

    m->comps[0][0] = z_near / r;
    m->comps[1][1] = z_near / t;
    m->comps[2][2] = ((z_far + z_near) / (z_far - z_near)) - 1.0;
    m->comps[2][3] = -1.0;
    m->comps[3][2] = (z_near * z_far) / (z_far - z_near);
    m->comps[3][3] = 0.0;
}

void mat4_t_ortho(mat4_t *m, float width, float height, float z_near, float z_far)
{
    mat4_t_identity(m);

    m->comps[0][0] =  2.0 / width;
    m->comps[1][1] = -2.0 / height;
    m->comps[2][2] = -2.0 / (z_far - z_near);
    m->comps[3][2] = -(z_far + z_near) / (z_far - z_near);
}

void mat4_t_ortho_reverse_z(mat4_t *m, float width, float height, float z_near, float z_far)
{
    mat4_t_identity(m);

    m->comps[0][0] =  2.0 / width;
    m->comps[1][1] = -2.0 / height;
    m->comps[2][2] = (2.0 / (z_far - z_near)) - 1.0;
    m->comps[3][2] = (z_far + z_near) / (z_far - z_near);
}

void mat4_t_pitch(mat4_t* m, float pitch)
{
    mat4_t_identity(m);
    float s = sinf(pitch * 3.14159265);
    float c = cosf(pitch * 3.14159265);

    m->rows[1].comps[1] = c;
    m->rows[1].comps[2] = s;
    m->rows[2].comps[1] = -s;
    m->rows[2].comps[2] = c;
}

void mat4_t_yaw(mat4_t* m, float yaw)
{
    mat4_t_identity(m);

    float s = sinf(yaw * 3.14159265);
    float c = cosf(yaw * 3.14159265);

    m->rows[0].comps[0] = c;
    m->rows[0].comps[2] = -s;
    m->rows[2].comps[0] = s;
    m->rows[2].comps[2] = c;
}

void mat4_t_vec4_t_mul(vec4_t *r, mat4_t *m, vec4_t *v)
{
    vec4_t res = {};

    res.x = r->x * m->rows[0].x + r->y * m->rows[1].x + r->z * m->rows[2].x + r->w * m->rows[3].x;
    res.y = r->x * m->rows[0].y + r->y * m->rows[1].y + r->z * m->rows[2].y + r->w * m->rows[3].y;
    res.z = r->x * m->rows[0].z + r->y * m->rows[1].z + r->z * m->rows[2].z + r->w * m->rows[3].z;
    res.w = r->x * m->rows[0].w + r->y * m->rows[1].w + r->z * m->rows[2].w + r->w * m->rows[3].w;

    *r = res;
}

void mat4_t_vec4_t_mul_fast(vec4_t *r, mat4_t *m, vec4_t *v)
{
    __m128 accum;
    accum = _mm_mul_ps(_mm_shuffle_ps(v->m128, v->m128, 0x00), m->rows[0].m128);
    accum = _mm_add_ps(accum, _mm_mul_ps(_mm_shuffle_ps(v->m128, v->m128, 0x55), m->rows[1].m128));
    accum = _mm_add_ps(accum, _mm_mul_ps(_mm_shuffle_ps(v->m128, v->m128, 0xaa), m->rows[2].m128));
    r->m128 = _mm_add_ps(accum, _mm_mul_ps(_mm_shuffle_ps(v->m128, v->m128, 0xff), m->rows[3].m128));
}

#ifdef __cplusplus
}
#endif

#endif

#endif // MATRIX_H





