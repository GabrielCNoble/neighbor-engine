#ifndef VECTOR_H
#define VECTOR_H

#include <math.h>
#include <intrin.h>

typedef struct vec2_t
{
    union
    {
        float comps[2];

        struct
        {
            float x;
            float y;
        };
    };

}vec2_t;

#define vec2_t_c(x, y) (vec2_t){{{x, y}}}

#ifdef __cplusplus
extern "C"
{
#endif

void vec2_t_add(vec2_t *r, vec2_t *a, vec2_t *b);

void vec2_t_sub(vec2_t *r, vec2_t *a, vec2_t *b);

float vec2_t_dot(vec2_t *a, vec2_t *b);

void vec2_t_mul(vec2_t *r, vec2_t *a, float s);

void vec2_t_normalize(vec2_t *r, vec2_t *v);

float vec2_t_length(vec2_t *v);

void vec2_t_lerp(vec2_t *r, vec2_t *a, vec2_t *b, float s);

#ifdef __cplusplus
}
#endif

/*
=====================================================================
=====================================================================
=====================================================================
*/

typedef struct vec3_t
{
    union
    {
        float comps[3];
        vec2_t xy;
        struct { float x, y, z; };
    };
}vec3_t;

#define vec3_t_c(x, y, z) (vec3_t){{{x, y, z}}}
#define vec3_t_c_vec4_t(v) (vec3_t){{{(v)->x, (v)->y, (v)->z}}}

#ifdef __cplusplus
extern "C"
{
#endif

void vec3_t_add(vec3_t *r, vec3_t *a, vec3_t *b);

void vec3_t_sub(vec3_t *r, vec3_t *a, vec3_t *b);

void vec3_t_mul(vec3_t *r, vec3_t *v, float s);

void vec3_t_div(vec3_t *r, vec3_t *v, float s);

void vec3_t_neg(vec3_t *r, vec3_t *v);

float vec3_t_length(vec3_t *v);

void vec3_t_normalize(vec3_t *r, vec3_t *v);

float vec3_t_dot(vec3_t *a, vec3_t *b);

void vec3_t_cross(vec3_t *r, vec3_t *a, vec3_t *b);

void vec3_t_fabs(vec3_t *r, vec3_t *v);

void vec3_t_fmadd(vec3_t *r, vec3_t *add, vec3_t *mul_a, float mul_b);

void vec3_t_lerp(vec3_t *r, vec3_t *a, vec3_t *b, float s);

void vec3_t_max(vec3_t *r, vec3_t *a, vec3_t *b);

void vec3_t_min(vec3_t *r, vec3_t *a, vec3_t *b);

void vec3_t_rotate_x(vec3_t *r, vec3_t *v, float angle);

void vec3_t_rotate_y(vec3_t *r, vec3_t *v, float angle);

void vec3_t_rotate_z(vec3_t *r, vec3_t *v, float angle);

#ifdef __cplusplus
}
#endif

/*
=====================================================================
=====================================================================
=====================================================================
*/

typedef struct vec4_t
{
    union
    {
        float comps[4];
        __m128 m128;
        vec3_t xyz;
        vec2_t xy;
        struct { float x, y, z, w; };
    };

}vec4_t;

#define vec4_t_c(x, y, z, w) (vec4_t){{{x, y, z, w}}}

#ifdef __cplusplus
extern "C"
{
#endif

void vec4_t_add(vec4_t *r, vec4_t *a, vec4_t *b);

void vec4_t_add_fast(vec4_t *r, vec4_t *a, vec4_t *b);

void vec4_t_sub(vec4_t *r, vec4_t *a, vec4_t *b);

void vec4_t_mul(vec4_t *r, vec4_t *v, float s);

void vec4_t_mul_fast(vec4_t *r, vec4_t *v, float s);

void vec4_t_div(vec4_t *r, vec4_t *v, float s);

void vec4_t_neg(vec4_t *r, vec4_t *v);

float vec4_t_length(vec4_t *v);

void vec4_t_normalize(vec4_t *r, vec4_t *v);

float vec4_t_dot(vec4_t *a, vec4_t *b);

void vec4_t_fabs(vec4_t *r, vec4_t *v);

void vec4_t_fmadd(vec4_t *r, vec4_t *add, vec4_t *mul_a, float mul_b);

void vec4_t_lerp(vec4_t *r, vec4_t *a, vec4_t *b, float t);

void quat_slerp(vec4_t *r, vec4_t *a, vec4_t *b, float t);

#ifdef __cplusplus
}
#endif

/*
=====================================================================
=====================================================================
=====================================================================
*/

#ifdef DS_VECTOR_IMPLEMENTATION

#ifdef __cplusplus
extern "C"
{
#endif

void vec2_t_add(vec2_t *r, vec2_t *a, vec2_t *b)
{
    r->x = a->x + b->x;
    r->y = a->y + b->y;
}

void vec2_t_sub(vec2_t *r, vec2_t *a, vec2_t *b)
{
    r->x = a->x - b->x;
    r->y = a->y - b->y;
}

float vec2_t_dot(vec2_t *a, vec2_t *b)
{
    return a->x * b->x + a->y * b->y;
}

void vec2_t_mul(vec2_t *r, vec2_t *a, float s)
{
    r->x = a->x * s;
    r->y = a->y * s;
}

void vec2_t_normalize(vec2_t *r, vec2_t *v)
{
    float l = sqrt(v->x * v->x + v->y * v->y);
    r->x = v->x / l;
    r->y = v->y / l;
}

float vec2_t_length(vec2_t *v)
{
    return sqrt(v->x * v->x + v->y * v->y);
}

void vec2_t_lerp(vec2_t *r, vec2_t *a, vec2_t *b, float s)
{
    r->x = a->x * (1.0 - s) + b->x * s;
    r->y = a->y * (1.0 - s) + b->y * s;
}




void vec3_t_add(vec3_t *r, vec3_t *a, vec3_t *b)
{
    r->x = a->x + b->x;
    r->y = a->y + b->y;
    r->z = a->z + b->z;
}

void vec3_t_sub(vec3_t *r, vec3_t *a, vec3_t *b)
{
    r->x = a->x - b->x;
    r->y = a->y - b->y;
    r->z = a->z - b->z;
}

void vec3_t_mul(vec3_t *r, vec3_t *v, float s)
{
    r->x = v->x * s;
    r->y = v->y * s;
    r->z = v->z * s;
}

void vec3_t_div(vec3_t *r, vec3_t *v, float s)
{
    r->x = v->x / s;
    r->y = v->y / s;
    r->z = v->z / s;
}

void vec3_t_neg(vec3_t *r, vec3_t *v)
{
    r->x = -v->x;
    r->y = -v->y;
    r->z = -v->z;
}

float vec3_t_length(vec3_t *v)
{
    return sqrt(v->x * v->x + v->y * v->y + v->z * v->z);
}

void vec3_t_normalize(vec3_t *r, vec3_t *v)
{
    float len = vec3_t_length(v);

    if(len)
    {
        vec3_t_div(r, v, len);
        return;
    }

    r->x = 0.0;
    r->y = 0.0;
    r->z = 0.0;
}

float vec3_t_dot(vec3_t *a, vec3_t *b)
{
    return a->x * b->x + a->y * b->y + a->z * b->z;
}

void vec3_t_cross(vec3_t *r, vec3_t *a, vec3_t *b)
{
    *r = (vec3_t){{.x = a->z * b->y - a->y * b->z, .y = a->x * b->z - a->z * b->x, .z = a->y * b->x - a->x * b->y}};
}

void vec3_t_fabs(vec3_t *r, vec3_t *v)
{
    r->x = fabsf(v->x);
    r->y = fabsf(v->y);
    r->z = fabsf(v->z);
}

void vec3_t_fmadd(vec3_t *r, vec3_t *add, vec3_t *mul_a, float mul_b)
{
    *r = (vec3_t)
    {
        .x = fmaf(mul_a->x, mul_b, add->x),
        .y = fmaf(mul_a->y, mul_b, add->y),
        .z = fmaf(mul_a->z, mul_b, add->z),
    };
}

void vec3_t_lerp(vec3_t *r, vec3_t *a, vec3_t *b, float s)
{
    r->x = a->x * (1.0 - s) + b->x * s;
    r->y = a->y * (1.0 - s) + b->y * s;
    r->z = a->z * (1.0 - s) + b->z * s;
}

void vec3_t_max(vec3_t *r, vec3_t *a, vec3_t *b)
{
    r->x = fmax(a->x, b->x);
    r->y = fmax(a->y, b->y);
    r->z = fmax(a->z, b->z);
}

void vec3_t_min(vec3_t *r, vec3_t *a, vec3_t *b)
{
    r->x = fmin(a->x, b->x);
    r->y = fmin(a->y, b->y);
    r->z = fmin(a->z, b->z);
}

void vec3_t_rotate_x(vec3_t *r, vec3_t *v, float angle)
{
    vec3_t t;
    float s;
    float c;

    s = sin(angle * 3.14159265);
    c = cos(angle * 3.14159265);

    t.x = v->x;
    t.y = v->y * c - v->z * s;
    t.z = v->z * c + v->y * s;

    *r = t;
}

void vec3_t_rotate_y(vec3_t *r, vec3_t *v, float angle)
{
    vec3_t t;
    float s;
    float c;

    s = sin(angle * 3.14159265);
    c = cos(angle * 3.14159265);

    t.x = v->x * c + v->z * s;
    t.y = v->y;
    t.z = v->z * c - v->x * s;

    *r = t;
}

void vec3_t_rotate_z(vec3_t *r, vec3_t *v, float angle)
{
    vec3_t t;
    float s;
    float c;

    s = sin(angle * 3.14159265);
    c = cos(angle * 3.14159265);

    t.x = v->x * c - v->y * s;
    t.y = v->y * c - v->x * s;
    t.z = v->z;

    *r = t;
}






void vec4_t_add(vec4_t *r, vec4_t *a, vec4_t *b)
{
    r->x = a->x + b->x;
    r->y = a->y + b->y;
    r->z = a->z + b->z;
    r->w = a->w + b->w;
}

void vec4_t_add_fast(vec4_t *r, vec4_t *a, vec4_t *b)
{
    r->m128 = _mm_add_ps(a->m128, b->m128);
}

void vec4_t_sub(vec4_t *r, vec4_t *a, vec4_t *b)
{
    r->x = a->x - b->x;
    r->y = a->y - b->y;
    r->z = a->z - b->z;
    r->w = a->w - b->w;
}

void vec4_t_mul(vec4_t *r, vec4_t *v, float s)
{
    r->x = v->x * s;
    r->y = v->y * s;
    r->z = v->z * s;
    r->w = v->w * s;
}

void vec4_t_mul_fast(vec4_t *r, vec4_t *v, float s)
{
    __m128 sv = _mm_load_ps1(&s);
    r->m128 = _mm_mul_ps(v->m128, sv);
}

void vec4_t_div(vec4_t *r, vec4_t *v, float s)
{
    r->x = v->x / s;
    r->y = v->y / s;
    r->z = v->z / s;
    r->w = v->w / s;
}

void vec4_t_neg(vec4_t *r, vec4_t *v)
{
    r->x = -v->x;
    r->y = -v->y;
    r->z = -v->z;
    r->w = -v->w;
}

float vec4_t_length(vec4_t *v)
{
    return sqrt(v->x * v->x + v->y * v->y + v->z * v->z + v->w * v->w);
}

void vec4_t_normalize(vec4_t *r, vec4_t *v)
{
    float len = vec4_t_length(v);

    if(len)
    {
        vec4_t_div(r, v, len);
        return;
    }

    r->x = 0.0;
    r->y = 0.0;
    r->z = 0.0;
    r->w = 0.0;
}

float vec4_t_dot(vec4_t *a, vec4_t *b)
{
    return a->x * b->x + a->y * b->y + a->z * b->z + a->w * b->w;
}

void vec4_t_fabs(vec4_t *r, vec4_t *v)
{
    r->x = fabsf(v->x);
    r->y = fabsf(v->y);
    r->z = fabsf(v->z);
    r->w = fabsf(v->w);
}

void vec4_t_fmadd(vec4_t *r, vec4_t *add, vec4_t *mul_a, float mul_b)
{
    *r = (vec4_t)
    {
        .x = fmaf(mul_a->x, mul_b, add->x),
        .y = fmaf(mul_a->y, mul_b, add->y),
        .z = fmaf(mul_a->z, mul_b, add->z),
        .w = fmaf(mul_a->w, mul_b, add->w),
    };
}

void vec4_t_lerp(vec4_t *r, vec4_t *a, vec4_t *b, float t)
{
    float at = 1.0 - t;
    *r = (vec4_t)
    {
        .x = a->x * at + b->x * t,
        .y = a->y * at + b->y * t,
        .z = a->z * at + b->z * t,
        .w = a->w * at + b->w * t,
    };
}

void quat_slerp(vec4_t *r, vec4_t *a, vec4_t *b, float t)
{
    /*
        implementation copied from: http://www.euclideanspace.com/maths/algebra/realNormedAlgebra/quaternions/slerp/index.htm
    */
    float cos_half_theta = a->x * b->x + a->y * b->y + a->z * b->z + a->w * b->w;
    vec4_t tb;

    if(cos_half_theta < 0.0)
    {
        tb.x = -b->x;
        tb.y = -b->y;
        tb.z = -b->z;
        tb.w = -b->w;
        b = &tb;

        cos_half_theta = -cos_half_theta;
    }

    if(fabs(cos_half_theta) >= 1.0)
    {
        *r = *a;
        return;
    }

    float half_theta = acos(cos_half_theta);
    float sin_half_theta = sqrt(1.0 - cos_half_theta * cos_half_theta);

    if(fabs(sin_half_theta) < 0.001)
    {
        r->x = a->x * 0.5 + b->x * 0.5;
        r->y = a->y * 0.5 + b->y * 0.5;
        r->z = a->z * 0.5 + b->z * 0.5;
        r->w = a->w * 0.5 + b->w * 0.5;
        return;
    }

    float ratio_a = sin((1.0 - t) * half_theta) / sin_half_theta;
    float ratio_b = sin(t * half_theta) / sin_half_theta;

    r->x = a->x * ratio_a + b->x * ratio_b;
    r->y = a->y * ratio_a + b->y * ratio_b;
    r->z = a->z * ratio_a + b->z * ratio_b;
    r->w = a->w * ratio_a + b->w * ratio_b;

    return;
}

#ifdef __cplusplus
}
#endif

#endif

#endif // VECTOR_H










