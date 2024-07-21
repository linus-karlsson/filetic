#pragma once

#define V2_FMT(v) "(x: %f, y: %f)\n", (v).x, (v).y
#define V3_FMT(v) "(x: %f, y: %f, z: %f)\n", (v).x, (v).y, (v).z
#define V4_FMT(v) "(x: %f, y: %f, z: %f, w: %f)\n", (v).x, (v).y, (v).z, (v).w

#define M3_FMT(m)                                                              \
    "|%f,%f,%f|\n|%f,%f,%f|\n|%f,%f,%f|\n\n", (m).data[0][0], (m).data[1][0],  \
        (m).data[2][0], (m).data[0][1], (m).data[1][1], (m).data[2][1],        \
        (m).data[0][2], (m).data[1][2], (m).data[2][2]

#define M4_FMT(m)                                                              \
    "|%f,%f,%f,%f|\n|%f,%f,%f,%f|\n|%f,%f,%f,%f|\n|%f,%f,%f,%f|\n\n",          \
        (m).data[0][0], (m).data[1][0], (m).data[2][0], (m).data[3][0],        \
        (m).data[0][1], (m).data[1][1], (m).data[2][1], (m).data[3][1],        \
        (m).data[0][2], (m).data[1][2], (m).data[2][2], (m).data[3][2],        \
        (m).data[0][3], (m).data[1][3], (m).data[2][3], (m).data[3][3]

typedef struct V2 V2;
struct V2
{
    union
    {
        struct
        {
            float x;
            float y;
        };

        struct
        {
            float width;
            float height;
        };

        struct
        {
            float min;
            float max;
        };

        struct
        {
            float u;
            float v;
        };

        struct
        {
            float l;
            float r;
        };
    };
};

typedef struct V3 V3;
struct V3
{
    union
    {
        struct
        {
            float x;
            float y;
            float z;
        };
        struct
        {
            float r;
            float g;
            float b;
        };
        struct
        {
            float data[3];
        };
    };
};

typedef struct V4 V4;
struct V4
{
    union
    {
        struct
        {
            float x;
            float y;
            float z;
            float w;
        };

        struct
        {
            float r;
            float g;
            float b;
            float a;
        };
    };
};

typedef struct Mat2f
{
    float data[2][2];
} Mat2f, M2;

typedef struct Mat3f
{
    float data[3][3];
} Mat3f, M3;

typedef struct Mat4f
{
    float data[4][4];
} Mat4f, M4;

V2 v2d(void);
V2 v2i(float i);
V2 v2f(float x, float y);
V2 v2_v3(V3 v3);
V2 v2_v4(V4 v4);
V3 v3d(void);
V3 v3i(float i);
V3 v3f(float x, float y, float z);
V3 v3_v2(V2 v2);
V3 v3_v2f(V2 v2, float z);
V3 v3_v4(V4 v4);
V4 v4d(void);
V4 v4i(float i);
V4 v4ic(float i);
V4 v4a(V4 v4, float a);
V4 v4f(float x, float y, float z, float w);
V4 v4_v2(V2 v2);
V4 v4_v2f(V2 v2, float z, float w);
V4 v4_v3(V3 v3);
V4 v4_v3f(V3 v3, float w);
float v2_sum(V2 v);
float v3_sum(V3 v);
float v4_sum(V4 v);
V2 v2_add(V2 v1, V2 v2);
V3 v3_add(V3 v1, V3 v2);
V4 v4_add(V4 v1, V4 v2);
V2 v2_sub(V2 v1, V2 v2);
V3 v3_sub(V3 v1, V3 v2);
V4 v4_sub(V4 v1, V4 v2);
V2 v2_div(V2 v1, V2 v2);
V3 v3_div(V3 v1, V3 v2);
V4 v4_div(V4 v1, V4 v2);
V2 v2_s_add(V2 v1, float s);
V3 v3_s_add(V3 v1, float s);
V4 v4_s_add(V4 v1, float s);
V2 v2_s_sub(V2 v1, float s);
V3 v3_s_sub(V3 v1, float s);
V4 v4_s_sub(V4 v1, float s);
V2 v2_s_multi(V2 v1, float s);
V3 v3_s_multi(V3 v1, float s);
V4 v4_s_multi(V4 v1, float s);
V2 v2_neg(V2 v);
V3 v3_neg(V3 v);
V4 v4_neg(V4 v);
V2 v2_multi(V2 v1, V2 v2);
V3 v3_multi(V3 v1, V3 v2);
V4 v4_multi(V4 v1, V4 v2);
V2 v2_s_div(V2 v1, float s);
V3 v3_s_div(V3 v1, float s);
V4 v4_s_div(V4 v1, float s);
void v2_add_equal(V2* v1, V2 v2);
void v3_add_equal(V3* v1, V3 v2);
void v4_add_equal(V4* v1, V4 v2);
void v2_sub_equal(V2* v1, V2 v2);
void v3_sub_equal(V3* v1, V3 v2);
void v4_sub_equal(V4* v1, V4 v2);
void v2_s_add_equal(V2* v1, float s);
void v3_s_add_equal(V3* v1, float s);
void v4_s_add_equal(V4* v1, float s);
void v2_s_sub_equal(V2* v1, float s);
void v3_s_sub_equal(V3* v1, float s);
void v4_s_sub_equal(V4* v1, float s);
void v2_s_multi_equal(V2* v1, float s);
void v3_s_multi_equal(V3* v1, float s);
void v4_s_multi_equal(V4* v1, float s);
void v2_s_div_equal(V2* v1, float s);
void v3_s_div_equal(V3* v1, float s);
void v4_s_div_equal(V4* v1, float s);
long v2_equal(V2 v1, V2 v2);
long v3_equal(V3 v1, V3 v2);
long v4_equal(V4 v1, V4 v2);
long v2_less(V2 v1, V2 v2);
long v3_less(V3 v1, V3 v2);
long v4_less(V4 v1, V4 v2);
long v2_more(V2 v1, V2 v2);
long v3_more(V3 v1, V3 v2);
long v4_more(V4 v1, V4 v2);
float v2_len(V2 v2);
float v2_len_squared(V2 v2);
float v3_len_squared(V3 v3);
float v3_len(V3 v3);
V3 v3_lerp(V3 v1, V3 v2, float t);
float v2_dot(V2 v1, V2 v2);
float v3_dot(V3 v1, V3 v2);
float v3_angle(V3 v1, V3 v2);
V2 v2_normalize(V2 v2);
V3 v3_normalize(V3 v3);
float v2_cross(V2 v1, V2 v2);
V3 v3_cross(V3 v1, V3 v2);
V3 v3_project(V3 v1, V3 v2);
V3 v3_reject(V3 v1, V3 v2);
V3 v3_rotate(V3 v3, float rad, V3 normal);
float v2_distance(V2 v1, V2 v2);
float v3_distance_squared(V3 v1, V3 v2);
float v3_distance(V3 v1, V3 v2);

M2 m2i(float i);
M2 m2d(void);
M3 m3i(float i);
M3 m3d(void);
M3 m3f(float f0, float f1, float f2, float f3, float f4, float f5, float f6,
       float f7, float f8);
M3 m3_m4(M4 matrix);
M4 m4i(float i);
M4 m4d(void);
M4 m4f(float f0, float f1, float f2, float f3, float f4, float f5, float f6,
       float f7, float f8, float f9, float f10, float f11, float f12, float f13,
       float f14, float f15);
M4 m4_v4(V4 c0, V4 c1, V4 c2, V4 c3);
float m2_sum(M2 m);
float m3_sum(M3 m);
float m4_sum(M4 m);
M2 m2_add(M2 m1, M2 m2);
M3 m3_add(M3 m1, M3 m2);
M4 m4_add(M4 m1, M4 m2);
M2 m2_sub(M2 m1, M2 m2);
M3 m3_sub(M3 m1, M3 m2);
M4 m4_sub(M4 m1, M4 m2);
M2 m2_s_multi(M2 m, float s);
M3 m3_s_multi(M3 m, float s);
M4 m4_s_multi(M4 m, float s);
V2 m2_v2_multi(M2 m, V2 v);
V3 m3_v3_multi(M3 m, V3 v);
V3 m4_v3_multi(M4 m, V3 v);
V4 m4_v4_multi(M4 m, V4 v);
M2 m2_multi(M2 m1, M2 m2);
M3 m3_multi(M3 m1, M3 m2);
M4 m4_multi(M4 m1, M4 m2);
M4 m4_s_div(M4 m, float s);
long m2_equal(M2 m1, M2 m2);
long m3_equal(M3 m1, M3 m2);
long m4_equal(M4 m1, M4 m2);
long m2_less(M2 m1, M2 m2);
long m3_less(M3 m1, M3 m2);
long m4_less(M4 m1, M4 m2);
long m2_more(M2 m1, M2 m2);
long m3_more(M3 m1, M3 m2);
long m4_more(M4 m1, M4 m2);
M3 m3_transpose(M3 m3);
M4 m4_transpose(M4 m4);
M3 m3_translate(V2 v);
M4 m4_translate(V3 v3);
M3 m3_scale(V2 v);
M4 m4_scale(V3 v);
M4 m4_shear(V3 v, V2 hx, V2 hy, V2 hz);
M4 ortho(float left, float right, float bottom, float top, float sy_near,
         float sy_far);
M4 view(V3 eye, V3 center, V3 up);
M4 perspective(float fov, float aspect, float sy_near, float sy_far);
M4 inverse(M4 m);

#ifdef FTIC_MATH_IMPLEMENTATION

V2 v2d(void)
{
    V2 res = { 0 };
    return res;
}

V2 v2i(float i)
{
    V2 res;
    res.x = i;
    res.y = i;
    return res;
}

V2 v2f(float x, float y)
{
    V2 res;
    res.x = x;
    res.y = y;
    return res;
}

V2 v2_v3(V3 v3)
{
    return v2f(v3.x, v3.y);
}

V2 v2_v4(V4 v4)
{
    return v2f(v4.x, v4.y);
}

V3 v3d(void)
{
    V3 res = { 0 };
    return res;
}

V3 v3i(float i)
{
    V3 res;
    res.x = i;
    res.y = i;
    res.z = i;
    return res;
}

V3 v3f(float x, float y, float z)
{
    V3 res;
    res.x = x;
    res.y = y;
    res.z = z;
    return res;
}

V3 v3_v2(V2 v2)
{
    return v3f(v2.x, v2.y, 0.0f);
}

V3 v3_v2f(V2 v2, float z)
{
    return v3f(v2.x, v2.y, z);
}

V3 v3_v4(V4 v4)
{
    return v3f(v4.x, v4.y, v4.z);
}

V4 v4d(void)
{
    V4 res = { 0 };
    return res;
}

V4 v4i(float i)
{
    V4 res;
    res.x = i;
    res.y = i;
    res.z = i;
    res.w = i;
    return res;
}

V4 v4ic(float i)
{
    V4 res;
    res.x = i;
    res.y = i;
    res.z = i;
    res.w = 1.0f;
    return res;
}

V4 v4a(V4 v4, float a)
{
    V4 res = v4;
    res.w = a;
    return res;
}

V4 v4f(float x, float y, float z, float w)
{
    V4 res;
    res.x = x;
    res.y = y;
    res.z = z;
    res.w = w;
    return res;
}

V4 v4_v2(V2 v2)
{
    return v4f(v2.x, v2.y, 0.0f, 0.0f);
}

V4 v4_v2f(V2 v2, float z, float w)
{
    return v4f(v2.x, v2.y, z, w);
}

V4 v4_v3(V3 v3)
{
    return v4f(v3.x, v3.y, v3.z, 0.0f);
}

V4 v4_v3f(V3 v3, float w)
{
    return v4f(v3.x, v3.y, v3.z, w);
}

float v2_sum(V2 v)
{
    return (v.x + v.y);
}

float v3_sum(V3 v)
{
    return (v.x + v.y + v.z);
}

float v4_sum(V4 v)
{
    return (v.x + v.y + v.z + v.w);
}

V2 v2_add(V2 v1, V2 v2)
{
    return v2f(v1.x + v2.x, v1.y + v2.y);
}

V3 v3_add(V3 v1, V3 v2)
{
    return v3f(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z);
}

V4 v4_add(V4 v1, V4 v2)
{
    return v4f(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z, v1.w + v2.w);
}

V2 v2_sub(V2 v1, V2 v2)
{
    return v2f(v1.x - v2.x, v1.y - v2.y);
}

V3 v3_sub(V3 v1, V3 v2)
{
    return v3f(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z);
}

V4 v4_sub(V4 v1, V4 v2)
{
    return v4f(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z, v1.w - v2.w);
}

V2 v2_div(V2 v1, V2 v2)
{
    return v2f(v1.x / v2.x, v1.y / v2.y);
}

V3 v3_div(V3 v1, V3 v2)
{
    return v3f(v1.x / v2.x, v1.y / v2.y, v1.z / v2.z);
}

V4 v4_div(V4 v1, V4 v2)
{
    return v4f(v1.x / v2.x, v1.y / v2.y, v1.z / v2.z, v1.w / v2.w);
}

V2 v2_s_add(V2 v1, float s)
{
    return v2f(v1.x + s, v1.y + s);
}

V3 v3_s_add(V3 v1, float s)
{
    return v3f(v1.x + s, v1.y + s, v1.z + s);
}

V4 v4_s_add(V4 v1, float s)
{
    return v4f(v1.x + s, v1.y + s, v1.z + s, v1.w + s);
}

V2 v2_s_sub(V2 v1, float s)
{
    return v2f(v1.x - s, v1.y - s);
}

V3 v3_s_sub(V3 v1, float s)
{
    return v3f(v1.x - s, v1.y - s, v1.z - s);
}

V4 v4_s_sub(V4 v1, float s)
{
    return v4f(v1.x - s, v1.y - s, v1.z - s, v1.w - s);
}

V2 v2_s_multi(V2 v1, float s)
{
    return v2f(v1.x * s, v1.y * s);
}

V3 v3_s_multi(V3 v1, float s)
{
    return v3f(v1.x * s, v1.y * s, v1.z * s);
}

V4 v4_s_multi(V4 v1, float s)
{
    return v4f(v1.x * s, v1.y * s, v1.z * s, v1.w * s);
}

V2 v2_neg(V2 v)
{
    return v2_s_multi(v, -1.0f);
}

V3 v3_neg(V3 v)
{
    return v3_s_multi(v, -1.0f);
}

V4 v4_neg(V4 v)
{
    return v4_s_multi(v, -1.0f);
}

V2 v2_multi(V2 v1, V2 v2)
{
    return v2f(v1.x * v2.x, v1.y * v2.y);
}

V3 v3_multi(V3 v1, V3 v2)
{
    return v3f(v1.x * v2.x, v1.y * v2.y, v1.z * v2.z);
}

V4 v4_multi(V4 v1, V4 v2)
{
    return v4f(v1.x * v2.x, v1.y * v2.y, v1.x * v2.y, v1.z);
}

V2 v2_s_div(V2 v1, float s)
{
    return v2f(v1.x / s, v1.y / s);
}

V3 v3_s_div(V3 v1, float s)
{
    return v3f(v1.x / s, v1.y / s, v1.z / s);
}

V4 v4_s_div(V4 v1, float s)
{
    return v4f(v1.x / s, v1.y / s, v1.z / s, v1.w / s);
}

void v2_add_equal(V2* v1, V2 v2)
{
    *v1 = v2_add(*v1, v2);
}

void v3_add_equal(V3* v1, V3 v2)
{
    *v1 = v3_add(*v1, v2);
}

void v4_add_equal(V4* v1, V4 v2)
{
    *v1 = v4_add(*v1, v2);
}

void v2_sub_equal(V2* v1, V2 v2)
{
    *v1 = v2_sub(*v1, v2);
}

void v3_sub_equal(V3* v1, V3 v2)
{
    *v1 = v3_sub(*v1, v2);
}

void v4_sub_equal(V4* v1, V4 v2)
{
    *v1 = v4_sub(*v1, v2);
}

void v2_s_add_equal(V2* v1, float s)
{
    *v1 = v2_s_add(*v1, s);
}

void v3_s_add_equal(V3* v1, float s)
{
    *v1 = v3_s_add(*v1, s);
}

void v4_s_add_equal(V4* v1, float s)
{
    *v1 = v4_s_add(*v1, s);
}

void v2_s_sub_equal(V2* v1, float s)
{
    *v1 = v2_s_sub(*v1, s);
}

void v3_s_sub_equal(V3* v1, float s)
{
    *v1 = v3_s_sub(*v1, s);
}

void v4_s_sub_equal(V4* v1, float s)
{
    *v1 = v4_s_sub(*v1, s);
}

void v2_s_multi_equal(V2* v1, float s)
{
    *v1 = v2_s_multi(*v1, s);
}

void v3_s_multi_equal(V3* v1, float s)
{
    *v1 = v3_s_multi(*v1, s);
}

void v4_s_multi_equal(V4* v1, float s)
{
    *v1 = v4_s_multi(*v1, s);
}

void v2_s_div_equal(V2* v1, float s)
{
    *v1 = v2_s_div(*v1, s);
}

void v3_s_div_equal(V3* v1, float s)
{
    *v1 = v3_s_div(*v1, s);
}

void v4_s_div_equal(V4* v1, float s)
{
    *v1 = v4_s_div(*v1, s);
}

long v2_equal(V2 v1, V2 v2)
{
    return v1.x == v2.x && v1.y == v2.y;
}

long v3_equal(V3 v1, V3 v2)
{
    return v1.x == v2.x && v1.y == v2.y && v1.z == v2.z;
}

long v4_equal(V4 v1, V4 v2)
{
    return v1.x == v2.x && v1.y == v2.y && v1.z == v2.z && v1.w == v2.w;
}

long v2_less(V2 v1, V2 v2)
{
    return (v2_sum(v1) < v2_sum(v2));
}

long v3_less(V3 v1, V3 v2)
{
    return (v3_sum(v1) < v3_sum(v2));
}

long v4_less(V4 v1, V4 v2)
{
    return (v4_sum(v1) < v4_sum(v2));
}

long v2_more(V2 v1, V2 v2)
{
    return (v2_sum(v1) > v2_sum(v2));
}

long v3_more(V3 v1, V3 v2)
{
    return (v3_sum(v1) > v3_sum(v2));
}

long v4_more(V4 v1, V4 v2)
{
    return (v4_sum(v1) > v4_sum(v2));
}

float v2_len(V2 v2)
{
    return sqrtf((v2.x * v2.x) + (v2.y * v2.y));
}

float v2_len_squared(V2 v2)
{
    return (v2.x * v2.x) + (v2.y * v2.y);
}

float v3_len_squared(V3 v3)
{
    return (v3.x * v3.x) + (v3.y * v3.y) + (v3.z * v3.z);
}

float v3_len(V3 v3)
{
    return sqrtf((v3.x * v3.x) + (v3.y * v3.y) + (v3.z * v3.z));
}

V3 v3_lerp(V3 v1, V3 v2, float t)
{
    return v3_add(v1, v3_s_multi(v3_sub(v2, v1), t));
}

float v2_dot(V2 v1, V2 v2)
{
    return (v1.x * v2.x) + (v1.y * v2.y);
}

float v3_dot(V3 v1, V3 v2)
{
    return ((v1.x * v2.x) + (v1.y * v2.y) + (v1.z * v2.z));
}

float v3_angle(V3 v1, V3 v2)
{
    float denominator =
        (1.0f / sqrtf(v3_dot(v1, v1))) * (1.0f / sqrtf(v3_dot(v2, v2)));
    return acosf(v3_dot(v1, v2) * denominator);
}

V2 v2_normalize(V2 v2)
{
    float inverse = 1 / sqrtf(v2_dot(v2, v2));
    V2 out = v2_s_multi(v2, inverse);
    return out;
}

V3 v3_normalize(V3 v3)
{
    float inverse = 1 / sqrtf(v3_dot(v3, v3));
    V3 out = v3_s_multi(v3, inverse);
    return out;
}

float v2_cross(V2 v1, V2 v2)
{
    float out = (v1.x * v2.y) - (v1.y * v2.x);
    return out;
}

V3 v3_cross(V3 v1, V3 v2)
{
    V3 out;

    out.x = (v1.y * v2.z) - (v1.z * v2.y);
    out.y = (v1.z * v2.x) - (v1.x * v2.z);
    out.z = (v1.x * v2.y) - (v1.y * v2.x);

    return out;
}

V3 v3_project(V3 v1, V3 v2)
{
    return v3_s_multi(v2, (v3_dot(v1, v2) / v3_dot(v2, v2)));
}

V3 v3_reject(V3 v1, V3 v2)
{
    return v3_sub(v1, v3_project(v1, v2));
}

V3 v3_rotate(V3 v3, float rad, V3 normal)
{
    float cos_ = cosf(rad);
    float sin_ = sinf(rad);

    return v3_add(
        v3_add(
            v3_s_multi(v3, cos_),
            v3_multi(v3_s_multi(v3_multi(v3, normal), (1.0f - cos_)), normal)),
        v3_s_multi(v3_cross(v3, normal), sin_));
}

float v2_distance(V2 v1, V2 v2)
{
    return v2_len(v2_sub(v1, v2));
}

float v3_distance_squared(V3 v1, V3 v2)
{
    return v3_len_squared(v3_sub(v1, v2));
}

float v3_distance(V3 v1, V3 v2)
{
    return v3_len(v3_sub(v1, v2));
}

M2 m2i(float i)
{
    M2 res = { 0 };
    res.data[0][0] = i;
    res.data[1][1] = i;
    return res;
}

M2 m2d(void)
{
    return m2i(1.0f);
}

M3 m3i(float i)
{
    M3 res = { 0 };
    res.data[0][0] = i;
    res.data[1][1] = i;
    res.data[2][2] = i;
    return res;
}

M3 m3d(void)
{
    return m3i(1.0f);
}

M3 m3f(float f0, float f1, float f2, float f3, float f4, float f5, float f6,
       float f7, float f8)
{
    M3 res;

    res.data[0][0] = f0;
    res.data[1][0] = f1;
    res.data[2][0] = f2;

    res.data[0][1] = f3;
    res.data[1][1] = f4;
    res.data[2][1] = f5;

    res.data[0][2] = f6;
    res.data[1][2] = f7;
    res.data[2][2] = f8;

    return res;
}

M3 m3_m4(M4 matrix)
{
    M3 res = { 0 };
    res.data[0][0] = matrix.data[0][0];
    res.data[0][1] = matrix.data[0][1];
    res.data[0][2] = matrix.data[0][2];

    res.data[1][0] = matrix.data[1][0];
    res.data[1][1] = matrix.data[1][1];
    res.data[1][2] = matrix.data[1][2];

    res.data[2][0] = matrix.data[2][0];
    res.data[2][1] = matrix.data[2][1];
    res.data[2][2] = matrix.data[2][2];

    return res;
}

M4 m4i(float i)
{
    M4 res = { 0 };
    res.data[0][0] = i;
    res.data[1][1] = i;
    res.data[2][2] = i;
    res.data[3][3] = i;
    return res;
}

M4 m4d(void)
{
    return m4i(1.0f);
}

M4 m4f(float f0, float f1, float f2, float f3, float f4, float f5, float f6,
       float f7, float f8, float f9, float f10, float f11, float f12, float f13,
       float f14, float f15)
{

    M4 res;
    res.data[0][0] = f0;
    res.data[0][1] = f4;
    res.data[0][2] = f8;
    res.data[0][3] = f12;

    res.data[1][0] = f1;
    res.data[1][1] = f5;
    res.data[1][2] = f9;
    res.data[1][3] = f13;

    res.data[2][0] = f2;
    res.data[2][1] = f6;
    res.data[2][2] = f10;
    res.data[2][3] = f14;

    res.data[3][0] = f3;
    res.data[3][1] = f7;
    res.data[3][2] = f11;
    res.data[3][3] = f15;
    return res;
}

M4 m4_v4(V4 c0, V4 c1, V4 c2, V4 c3)
{
    M4 res;
    res.data[0][0] = c0.x;
    res.data[0][1] = c0.y;
    res.data[0][2] = c0.z;
    res.data[0][3] = c0.w;

    res.data[1][0] = c1.x;
    res.data[1][1] = c1.y;
    res.data[1][2] = c1.z;
    res.data[1][3] = c1.w;

    res.data[2][0] = c2.x;
    res.data[2][1] = c2.y;
    res.data[2][2] = c2.z;
    res.data[2][3] = c2.w;

    res.data[3][0] = c3.x;
    res.data[3][1] = c3.y;
    res.data[3][2] = c3.z;
    res.data[3][3] = c3.w;
    return res;
}

float m2_sum(M2 m)
{
    float sum = 0.0f;

    for (int c = 0; c < 2; c++)
        for (int r = 0; r < 2; r++)
            sum += m.data[c][r];

    return sum;
}

float m3_sum(M3 m)
{
    float sum = 0.0f;

    for (int c = 0; c < 3; c++)
        for (int r = 0; r < 3; r++)
            sum += m.data[c][r];

    return sum;
}

float m4_sum(M4 m)
{
    float sum = 0.0f;

    for (int c = 0; c < 4; c++)
        for (int r = 0; r < 4; r++)
            sum += m.data[c][r];

    return sum;
}

M2 m2_add(M2 m1, M2 m2)
{
    m1.data[0][0] += m2.data[0][0];
    m1.data[0][1] += m2.data[0][1];

    m1.data[1][0] += m2.data[1][0];
    m1.data[1][1] += m2.data[1][1];

    return m1;
}

M3 m3_add(M3 m1, M3 m2)
{
    m1.data[0][0] += m2.data[0][0];
    m1.data[0][1] += m2.data[0][1];
    m1.data[0][2] += m2.data[0][2];

    m1.data[1][0] += m2.data[1][0];
    m1.data[1][1] += m2.data[1][1];
    m1.data[1][2] += m2.data[1][2];

    m1.data[2][0] += m2.data[2][0];
    m1.data[2][1] += m2.data[2][1];
    m1.data[2][2] += m2.data[2][2];

    return m1;
}

M4 m4_add(M4 m1, M4 m2)
{
    m1.data[0][0] += m2.data[0][0];
    m1.data[0][1] += m2.data[0][1];
    m1.data[0][2] += m2.data[0][2];
    m1.data[0][3] += m2.data[0][3];

    m1.data[1][0] += m2.data[1][0];
    m1.data[1][1] += m2.data[1][1];
    m1.data[1][2] += m2.data[1][2];
    m1.data[1][3] += m2.data[1][3];

    m1.data[2][0] += m2.data[2][0];
    m1.data[2][1] += m2.data[2][1];
    m1.data[2][2] += m2.data[2][2];
    m1.data[2][3] += m2.data[2][3];

    m1.data[3][0] += m2.data[3][0];
    m1.data[3][1] += m2.data[3][1];
    m1.data[3][2] += m2.data[3][2];
    m1.data[3][3] += m2.data[3][3];

    return m1;
}

M2 m2_sub(M2 m1, M2 m2)
{
    m1.data[0][0] -= m2.data[0][0];
    m1.data[0][1] -= m2.data[0][1];

    m1.data[1][0] -= m2.data[1][0];
    m1.data[1][1] -= m2.data[1][1];

    return m1;
}

M3 m3_sub(M3 m1, M3 m2)
{
    m1.data[0][0] -= m2.data[0][0];
    m1.data[0][1] -= m2.data[0][1];
    m1.data[0][2] -= m2.data[0][2];

    m1.data[1][0] -= m2.data[1][0];
    m1.data[1][1] -= m2.data[1][1];
    m1.data[1][2] -= m2.data[1][2];

    m1.data[2][0] -= m2.data[2][0];
    m1.data[2][1] -= m2.data[2][1];
    m1.data[2][2] -= m2.data[2][2];

    return m1;
}

M4 m4_sub(M4 m1, M4 m2)
{
    m1.data[0][0] -= m2.data[0][0];
    m1.data[0][1] -= m2.data[0][1];
    m1.data[0][2] -= m2.data[0][2];
    m1.data[0][3] -= m2.data[0][3];

    m1.data[1][0] -= m2.data[1][0];
    m1.data[1][1] -= m2.data[1][1];
    m1.data[1][2] -= m2.data[1][2];
    m1.data[1][3] -= m2.data[1][3];

    m1.data[2][0] -= m2.data[2][0];
    m1.data[2][1] -= m2.data[2][1];
    m1.data[2][2] -= m2.data[2][2];
    m1.data[2][3] -= m2.data[2][3];

    m1.data[3][0] -= m2.data[3][0];
    m1.data[3][1] -= m2.data[3][1];
    m1.data[3][2] -= m2.data[3][2];
    m1.data[3][3] -= m2.data[3][3];

    return m1;
}

M2 m2_s_multi(M2 m, float s)
{
    m.data[0][0] *= s;
    m.data[0][1] *= s;

    m.data[1][0] *= s;
    m.data[1][1] *= s;

    return m;
}

M3 m3_s_multi(M3 m, float s)
{
    m.data[0][0] *= s;
    m.data[0][1] *= s;
    m.data[0][2] *= s;

    m.data[1][0] *= s;
    m.data[1][1] *= s;
    m.data[1][2] *= s;

    m.data[2][0] *= s;
    m.data[2][1] *= s;
    m.data[2][2] *= s;

    return m;
}

M4 m4_s_multi(M4 m, float s)
{
    m.data[0][0] *= s;
    m.data[0][1] *= s;
    m.data[0][2] *= s;
    m.data[0][3] *= s;

    m.data[1][0] *= s;
    m.data[1][1] *= s;
    m.data[1][2] *= s;
    m.data[1][3] *= s;

    m.data[2][0] *= s;
    m.data[2][1] *= s;
    m.data[2][2] *= s;
    m.data[2][3] *= s;

    m.data[3][0] *= s;
    m.data[3][1] *= s;
    m.data[3][2] *= s;
    m.data[3][3] *= s;

    return m;
}

V2 m2_v2_multi(M2 m, V2 v)
{
    V2 out;
    out.x = (m.data[0][0] * v.x) + (m.data[1][0] * v.y);
    out.y = (m.data[0][1] * v.x) + (m.data[1][1] * v.y);
    return out;
}

V3 m3_v3_multi(M3 m, V3 v)
{
    V3 out;
    out.x = (m.data[0][0] * v.x) + (m.data[1][0] * v.y) + (m.data[2][0] * v.z);
    out.y = (m.data[0][1] * v.x) + (m.data[1][1] * v.y) + (m.data[2][1] * v.z);
    out.z = (m.data[0][2] * v.x) + (m.data[1][2] * v.y) + (m.data[2][2] * v.z);
    return out;
}

V3 m4_v3_multi(M4 m, V3 v)
{
    V3 out;
    out.x = (m.data[0][0] * v.x) + (m.data[1][0] * v.y) + (m.data[2][0] * v.z) +
            (m.data[3][0] * 1.0f);

    out.y = (m.data[0][1] * v.x) + (m.data[1][1] * v.y) + (m.data[2][1] * v.z) +
            (m.data[3][1] * 1.0f);

    out.z = (m.data[0][2] * v.x) + (m.data[1][2] * v.y) + (m.data[2][2] * v.z) +
            (m.data[3][2] * 1.0f);

    return out;
}

V4 m4_v4_multi(M4 m, V4 v)
{
    V4 out;
    out.x = (m.data[0][0] * v.x) + (m.data[1][0] * v.y) + (m.data[2][0] * v.z) +
            (m.data[3][0] * v.w);

    out.y = (m.data[0][1] * v.x) + (m.data[1][1] * v.y) + (m.data[2][1] * v.z) +
            (m.data[3][1] * v.w);

    out.z = (m.data[0][2] * v.x) + (m.data[1][2] * v.y) + (m.data[2][2] * v.z) +
            (m.data[3][2] * v.w);

    out.w = (m.data[0][3] * v.x) + (m.data[1][3] * v.y) + (m.data[2][3] * v.z) +
            (m.data[3][3] * v.w);
    return out;
}

M2 m2_multi(M2 m1, M2 m2)
{
    M2 out = { 0 };

    for (unsigned int col = 0; col < 2; col++)
        for (unsigned int row = 0; row < 2; row++)
            for (unsigned int i = 0; i < 2; i++)
                out.data[col][row] += m1.data[i][row] * m2.data[col][i];

    return out;
}

M3 m3_multi(M3 m1, M3 m2)
{
    M3 out = m3f(m1.data[0][0] * m2.data[0][0] + m1.data[1][0] * m2.data[0][1] +
                     m1.data[2][0] * m2.data[0][2],
                 m1.data[0][0] * m2.data[1][0] + m1.data[1][0] * m2.data[1][1] +
                     m1.data[2][0] * m2.data[1][2],
                 m1.data[0][0] * m2.data[2][0] + m1.data[1][0] * m2.data[2][1] +
                     m1.data[2][0] * m2.data[2][2],

                 m1.data[0][1] * m2.data[0][0] + m1.data[1][1] * m2.data[0][1] +
                     m1.data[2][1] * m2.data[0][2],
                 m1.data[0][1] * m2.data[1][0] + m1.data[1][1] * m2.data[1][1] +
                     m1.data[2][1] * m2.data[1][2],
                 m1.data[0][1] * m2.data[2][0] + m1.data[1][1] * m2.data[2][1] +
                     m1.data[2][1] * m2.data[2][2],

                 m1.data[0][2] * m2.data[0][0] + m1.data[1][2] * m2.data[0][1] +
                     m1.data[2][2] * m2.data[0][2],
                 m1.data[0][2] * m2.data[1][0] + m1.data[1][2] * m2.data[1][1] +
                     m1.data[2][2] * m2.data[1][2],
                 m1.data[0][2] * m2.data[2][0] + m1.data[1][2] * m2.data[2][1] +
                     m1.data[2][2] * m2.data[2][2]);

    return out;
}

M4 m4_multi(M4 m1, M4 m2)
{
    M4 out =
        m4f(m1.data[0][0] * m2.data[0][0] + m1.data[1][0] * m2.data[0][1] +
                m1.data[2][0] * m2.data[0][2] + m1.data[3][0] * m2.data[0][3],
            m1.data[0][0] * m2.data[1][0] + m1.data[1][0] * m2.data[1][1] +
                m1.data[2][0] * m2.data[1][2] + m1.data[3][0] * m2.data[1][3],
            m1.data[0][0] * m2.data[2][0] + m1.data[1][0] * m2.data[2][1] +
                m1.data[2][0] * m2.data[2][2] + m1.data[3][0] * m2.data[2][3],
            m1.data[0][0] * m2.data[3][0] + m1.data[1][0] * m2.data[3][1] +
                m1.data[2][0] * m2.data[3][2] + m1.data[3][0] * m2.data[3][3],

            m1.data[0][1] * m2.data[0][0] + m1.data[1][1] * m2.data[0][1] +
                m1.data[2][1] * m2.data[0][2] + m1.data[3][1] * m2.data[0][3],
            m1.data[0][1] * m2.data[1][0] + m1.data[1][1] * m2.data[1][1] +
                m1.data[2][1] * m2.data[1][2] + m1.data[3][1] * m2.data[1][3],
            m1.data[0][1] * m2.data[2][0] + m1.data[1][1] * m2.data[2][1] +
                m1.data[2][1] * m2.data[2][2] + m1.data[3][1] * m2.data[2][3],
            m1.data[0][1] * m2.data[3][0] + m1.data[1][1] * m2.data[3][1] +
                m1.data[2][1] * m2.data[3][2] + m1.data[3][1] * m2.data[3][3],

            m1.data[0][2] * m2.data[0][0] + m1.data[1][2] * m2.data[0][1] +
                m1.data[2][2] * m2.data[0][2] + m1.data[3][2] * m2.data[0][3],
            m1.data[0][2] * m2.data[1][0] + m1.data[1][2] * m2.data[1][1] +
                m1.data[2][2] * m2.data[1][2] + m1.data[3][2] * m2.data[1][3],
            m1.data[0][2] * m2.data[2][0] + m1.data[1][2] * m2.data[2][1] +
                m1.data[2][2] * m2.data[2][2] + m1.data[3][2] * m2.data[2][3],
            m1.data[0][2] * m2.data[3][0] + m1.data[1][2] * m2.data[3][1] +
                m1.data[2][2] * m2.data[3][2] + m1.data[3][2] * m2.data[3][3],

            m1.data[0][3] * m2.data[0][0] + m1.data[1][3] * m2.data[0][1] +
                m1.data[2][3] * m2.data[0][2] + m1.data[3][3] * m2.data[0][3],
            m1.data[0][3] * m2.data[1][0] + m1.data[1][3] * m2.data[1][1] +
                m1.data[2][3] * m2.data[1][2] + m1.data[3][3] * m2.data[1][3],
            m1.data[0][3] * m2.data[2][0] + m1.data[1][3] * m2.data[2][1] +
                m1.data[2][3] * m2.data[2][2] + m1.data[3][3] * m2.data[2][3],
            m1.data[0][3] * m2.data[3][0] + m1.data[1][3] * m2.data[3][1] +
                m1.data[2][3] * m2.data[3][2] + m1.data[3][3] * m2.data[3][3]);

    return out;
}

M4 m4_s_div(M4 m, float s)
{
    M4 out;
    out.data[0][0] = m.data[0][0] / s;
    out.data[0][1] = m.data[0][1] / s;
    out.data[0][2] = m.data[0][2] / s;
    out.data[0][3] = m.data[0][3] / s;

    out.data[1][0] = m.data[1][0] / s;
    out.data[1][1] = m.data[1][1] / s;
    out.data[1][2] = m.data[1][2] / s;
    out.data[1][3] = m.data[1][3] / s;

    out.data[2][0] = m.data[2][0] / s;
    out.data[2][1] = m.data[2][1] / s;
    out.data[2][2] = m.data[2][2] / s;
    out.data[2][3] = m.data[2][3] / s;

    out.data[3][0] = m.data[3][0] / s;
    out.data[3][1] = m.data[3][1] / s;
    out.data[3][2] = m.data[3][2] / s;
    out.data[3][3] = m.data[3][3] / s;
    return out;
}
long m2_equal(M2 m1, M2 m2)
{
    for (int c = 0; c < 2; c++)
        for (int r = 0; r < 2; r++)
            if (m1.data[c][r] != m2.data[c][r]) return 0;

    return 1;
}

long m3_equal(M3 m1, M3 m2)
{
    for (int c = 0; c < 3; c++)
        for (int r = 0; r < 3; r++)
            if (m1.data[c][r] != m2.data[c][r]) return 0;

    return 1;
}

long m4_equal(M4 m1, M4 m2)
{
    for (int c = 0; c < 4; c++)
        for (int r = 0; r < 4; r++)
            if (m1.data[c][r] != m2.data[c][r]) return 0;

    return 1;
}

long m2_less(M2 m1, M2 m2)
{
    return (m2_sum(m1) < m2_sum(m2));
}

long m3_less(M3 m1, M3 m2)
{
    return (m3_sum(m1) < m3_sum(m2));
}

long m4_less(M4 m1, M4 m2)
{
    return (m4_sum(m1) < m4_sum(m2));
}

long m2_more(M2 m1, M2 m2)
{
    return (m2_sum(m1) > m2_sum(m2));
}

long m3_more(M3 m1, M3 m2)
{
    return (m3_sum(m1) > m3_sum(m2));
}

long m4_more(M4 m1, M4 m2)
{
    return (m4_sum(m1) > m4_sum(m2));
}

M3 m3_transpose(M3 m3)
{
    M3 out;

    out.data[0][0] = m3.data[0][0];
    out.data[0][1] = m3.data[1][0];
    out.data[0][2] = m3.data[2][0];

    out.data[1][0] = m3.data[0][1];
    out.data[1][1] = m3.data[1][1];
    out.data[1][2] = m3.data[2][1];

    out.data[2][0] = m3.data[0][2];
    out.data[2][1] = m3.data[1][2];
    out.data[2][2] = m3.data[2][2];

    return out;
}

M4 m4_transpose(M4 m4)
{
    M4 out;

    out.data[0][0] = m4.data[0][0];
    out.data[0][1] = m4.data[1][0];
    out.data[0][2] = m4.data[2][0];
    out.data[0][3] = m4.data[3][0];

    out.data[1][0] = m4.data[0][1];
    out.data[1][1] = m4.data[1][1];
    out.data[1][2] = m4.data[2][1];
    out.data[1][3] = m4.data[3][1];

    out.data[2][0] = m4.data[0][2];
    out.data[2][1] = m4.data[1][2];
    out.data[2][2] = m4.data[2][2];
    out.data[2][3] = m4.data[3][2];

    out.data[3][0] = m4.data[0][3];
    out.data[3][1] = m4.data[1][3];
    out.data[3][2] = m4.data[2][3];
    out.data[3][3] = m4.data[3][3];

    return out;
}

M3 m3_translate(V2 v)
{
    M3 out = m3i(1.0f);
    out.data[2][0] = v.x;
    out.data[2][1] = v.y;
    return out;
}

M4 m4_translate(V3 v3)
{
    M4 out = m4i(1.0f);
    out.data[3][0] = v3.x;
    out.data[3][1] = v3.y;
    out.data[3][2] = v3.z;
    return out;
}

M3 m3_scale(V2 v)
{
    M3 out = m3i(1.0f);

    out.data[0][0] *= v.x;
    out.data[1][1] *= v.y;

    return out;
}

M4 m4_scale(V3 v)
{
    M4 out = m4i(1.0f);

    out.data[0][0] *= v.x;
    out.data[1][1] *= v.y;
    out.data[2][2] *= v.z;

    return out;
}

M4 m4_shear(V3 v, V2 hx, V2 hy, V2 hz)
{
    M4 out = m4i(1.0f);

    out.data[1][0] = hx.x;
    out.data[2][0] = hx.y;

    out.data[0][1] = hy.x;
    out.data[2][1] = hy.y;

    out.data[0][2] = hz.x;
    out.data[1][2] = hz.y;

    return out;
}

M4 ortho(float left, float right, float bottom, float top, float sy_near, float sy_far)
{
    M4 out = m4i(1.0f);

    out.data[0][0] = 2.0f / (right - left);
    out.data[1][1] = 2.0f / (top - bottom);
    out.data[2][2] = 2.0f / (sy_near - sy_far);

    out.data[3][0] = (left + right) / (left - right);
    out.data[3][1] = (bottom + top) / (bottom - top);
    out.data[3][2] = (sy_near + sy_far) / (sy_near - sy_far);

    return out;
}

M4 view(V3 eye, V3 center, V3 up)
{
    M4 out = m4i(1.0f);

    const V3 temp1 = v3_normalize(v3_sub(center, eye));
    const V3 temp2 = v3_normalize(v3_cross(temp1, up));
    const V3 temp3 = v3_cross(temp2, temp1);

    out.data[0][0] = temp2.x;
    out.data[0][1] = temp3.x;
    out.data[0][2] = -temp1.x;

    out.data[1][0] = temp2.y;
    out.data[1][1] = temp3.y;
    out.data[1][2] = -temp1.y;

    out.data[2][1] = temp3.z;
    out.data[2][0] = temp2.z;
    out.data[2][2] = -temp1.z;

    out.data[3][0] = -v3_dot(temp2, eye);
    out.data[3][1] = -v3_dot(temp3, eye);
    out.data[3][2] = v3_dot(temp1, eye);

    return out;
}

M4 perspective(float fov, float aspect, float sy_near, float sy_far)
{
    const float f = 1.0f / tanf((fov * 0.5f));
    const float X = f / aspect;
    const float Y = f;
    const float Z1 = (sy_far + sy_near) / (sy_near - sy_far);
    const float Z2 = (2.0f * sy_far * sy_near) / (sy_near - sy_far);

    M4 out = m4f(X, 0, 0, 0, 0, Y, 0, 0, 0, 0, Z1, Z2, 0, 0, -1.0f, 0.0f);

    return out;
}

M4 inverse(M4 m)
{
    float sf00 = m.data[2][2] * m.data[3][3] - m.data[3][2] * m.data[2][3];
    float sf01 = m.data[2][1] * m.data[3][3] - m.data[3][1] * m.data[2][3];
    float sf02 = m.data[2][1] * m.data[3][2] - m.data[3][1] * m.data[2][2];
    float sf03 = m.data[2][0] * m.data[3][3] - m.data[3][0] * m.data[2][3];
    float sf04 = m.data[2][0] * m.data[3][2] - m.data[3][0] * m.data[2][2];
    float sf05 = m.data[2][0] * m.data[3][1] - m.data[3][0] * m.data[2][1];
    float sf06 = m.data[1][2] * m.data[3][3] - m.data[3][2] * m.data[1][3];
    float sf07 = m.data[1][1] * m.data[3][3] - m.data[3][1] * m.data[1][3];
    float sf08 = m.data[1][1] * m.data[3][2] - m.data[3][1] * m.data[1][2];
    float sf09 = m.data[1][0] * m.data[3][3] - m.data[3][0] * m.data[1][3];
    float sf10 = m.data[1][0] * m.data[3][2] - m.data[3][0] * m.data[1][2];
    float sf11 = m.data[1][0] * m.data[3][1] - m.data[3][0] * m.data[1][1];
    float sf12 = m.data[1][2] * m.data[2][3] - m.data[2][2] * m.data[1][3];
    float sf13 = m.data[1][1] * m.data[2][3] - m.data[2][1] * m.data[1][3];
    float sf14 = m.data[1][1] * m.data[2][2] - m.data[2][1] * m.data[1][2];
    float sf15 = m.data[1][0] * m.data[2][3] - m.data[2][0] * m.data[1][3];
    float sf16 = m.data[1][0] * m.data[2][2] - m.data[2][0] * m.data[1][2];
    float sf17 = m.data[1][0] * m.data[2][1] - m.data[2][0] * m.data[1][1];

    M4 res = { 0 };
    res.data[0][0] =
        +(m.data[1][1] * sf00 - m.data[1][2] * sf01 + m.data[1][3] * sf02);
    res.data[0][1] =
        -(m.data[1][0] * sf00 - m.data[1][2] * sf03 + m.data[1][3] * sf04);
    res.data[0][2] =
        +(m.data[1][0] * sf01 - m.data[1][1] * sf03 + m.data[1][3] * sf05);
    res.data[0][3] =
        -(m.data[1][0] * sf02 - m.data[1][1] * sf04 + m.data[1][2] * sf05);

    res.data[1][0] =
        -(m.data[0][1] * sf00 - m.data[0][2] * sf01 + m.data[0][3] * sf02);
    res.data[1][1] =
        +(m.data[0][0] * sf00 - m.data[0][2] * sf03 + m.data[0][3] * sf04);
    res.data[1][2] =
        -(m.data[0][0] * sf01 - m.data[0][1] * sf03 + m.data[0][3] * sf05);
    res.data[1][3] =
        +(m.data[0][0] * sf02 - m.data[0][1] * sf04 + m.data[0][2] * sf05);

    res.data[2][0] =
        +(m.data[0][1] * sf06 - m.data[0][2] * sf07 + m.data[0][3] * sf08);
    res.data[2][1] =
        -(m.data[0][0] * sf06 - m.data[0][2] * sf09 + m.data[0][3] * sf10);
    res.data[2][2] =
        +(m.data[0][0] * sf07 - m.data[0][1] * sf09 + m.data[0][3] * sf11);
    res.data[2][3] =
        -(m.data[0][0] * sf08 - m.data[0][1] * sf10 + m.data[0][2] * sf11);

    res.data[3][0] =
        -(m.data[0][1] * sf12 - m.data[0][2] * sf13 + m.data[0][3] * sf14);
    res.data[3][1] =
        +(m.data[0][0] * sf12 - m.data[0][2] * sf15 + m.data[0][3] * sf16);
    res.data[3][2] =
        -(m.data[0][0] * sf13 - m.data[0][1] * sf15 + m.data[0][3] * sf17);
    res.data[3][3] =
        +(m.data[0][0] * sf14 - m.data[0][1] * sf16 + m.data[0][2] * sf17);

    float d = +m.data[0][0] * res.data[0][0] + m.data[0][1] * res.data[0][1] +
            m.data[0][2] * res.data[0][2] + m.data[0][3] * res.data[0][3];

    res = m4_s_div(res, d);

    // Column major
    return m4_transpose(res);
}

#endif
