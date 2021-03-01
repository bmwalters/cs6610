#include "matvec.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <tgmath.h>

/* column-major order */

void matmul(mat4 *outm, const mat4 *lm, const mat4 *rm) {
    const float *l = &lm->m[0][0];
    const float *r = &rm->m[0][0];
    float *out = &outm->m[0][0];
    for (int i = 0; i < 16; i += 4, out += 4) {
        float a[4], b[4], c[4], d[4];
        for (int j = 0; j < 4; ++j)
            a[j] = l[j] * r[i];
        for (int j = 0; j < 4; ++j)
            b[j] = l[4 + j] * r[i + 1];
        for (int j = 0; j < 4; ++j)
            c[j] = l[8 + j] * r[i + 2];
        for (int j = 0; j < 4; ++j)
            d[j] = l[12 + j] * r[i + 3];
        for (int j = 0; j < 4; ++j)
            out[j] = a[j] + b[j] + c[j] + d[j];
    }
}

void matscale(mat4 *out, float scale) {
    float m[4][4] = {
        {scale, 0, 0, 0}, {0, scale, 0, 0}, {0, 0, scale, 0}, {0, 0, 0, 1}};
    memcpy(out->m, m, sizeof(float[4][4]));
}

void mattranslate(mat4 *out, float tx, float ty, float tz) {
    float m[4][4] = {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {tx, ty, tz, 1}};
    memcpy(out->m, m, sizeof(float[4][4]));
}

void matrotatex(mat4 *out, float theta) {
    float m[4][4] = {{1, 0, 0, 0},
                     {0, cos(theta), -sin(theta), 0},
                     {0, sin(theta), cos(theta), 0},
                     {0, 0, 0, 1}};
    memcpy(out->m, m, sizeof(float[4][4]));
}

void matrotatey(mat4 *out, float theta) {
    float m[4][4] = {{cos(theta), 0, sin(theta), 0},
                     {0, 1, 0, 0},
                     {-sin(theta), 0, cos(theta), 0},
                     {0, 0, 0, 1}};
    memcpy(out->m, m, sizeof(float[4][4]));
}

void matrotatez(mat4 *out, float theta) {
    float m[4][4] = {{cos(theta), -sin(theta), 0, 0},
                     {sin(theta), cos(theta), 0, 0},
                     {0, 0, 1, 0},
                     {0, 0, 0, 1}};
    memcpy(out->m, m, sizeof(float[4][4]));
}

float vecmag(const vec3 v) {
    return sqrt(pow(v[0], 2) + pow(v[1], 2) + pow(v[2], 2));
}

static void assert_close(float a, float b) {
    const float epsilon = 0.1f;
    assert((a - b) <= epsilon);
}

static void test_vecmag() {
    vec3 v = {2, 3, 6};
    assert_close(vecmag(v), 7);
}

void vecnormalize(vec3 v) {
    float mag = vecmag(v);
    v[0] /= mag;
    v[1] /= mag;
    v[2] /= mag;
}

void vecscale(vec3 v, float scale) {
    v[0] *= scale;
    v[1] *= scale;
    v[2] *= scale;
}

float vecdot(const vec3 u, const vec3 v) {
    return u[0] * v[0] + u[1] * v[1] + u[2] * v[2];
}

void veccross(vec3 out, const vec3 u, const vec3 v) {
    out[0] = u[1] * v[2] - u[2] * v[1];
    out[1] = u[2] * v[0] - u[0] * v[2];
    out[2] = u[0] * v[1] - u[1] * v[0];
}

static void test_veccross() {
    vec3 out;

    vec3 u1 = {1, 0, 0};
    vec3 v1 = {0, 1, 0};
    veccross(out, u1, v1);
    assert_close(out[0], 0);
    assert_close(out[1], 0);
    assert_close(out[2], 1);

    vec3 u2 = {-7, 3, 15};
    vec3 v2 = {38, -3, -1};
    veccross(out, u2, v2);
    assert_close(out[0], 42);
    assert_close(out[1], 563);
    assert_close(out[2], -93);
}

void matview(mat4 *out, const vec3 eye, const vec3 target, const vec3 up) {
    float f[3] = {target[0] - eye[0], target[1] - eye[1], target[2] - eye[2]};
    vecnormalize(f);

    float s[3];
    veccross(s, f, up);
    vecnormalize(s);

    float u[3];
    veccross(u, s, f);

    float m[4][4] = {{s[0], u[0], -f[0], 0},
                     {s[1], u[1], -f[1], 0},
                     {s[2], u[2], -f[2], 0},
                     {-vecdot(s, eye), -vecdot(u, eye), vecdot(f, eye), 1}};
    memcpy(out->m, m, sizeof(float[4][4]));
}

void veceulerangles(vec3 out, float yaw, float pitch) {
    out[0] = cos(yaw) * cos(pitch);
    out[1] = sin(pitch);
    out[2] = sin(yaw) * cos(pitch);
}

void matperspective(mat4 *out, float fovy, float aspect, float z_near,
                    float z_far) {
    const float S = 1 / tan(fovy / 2);
    const float m[4][4] = {{S / aspect, 0, 0, 0},
                           {0, S, 0, 0},
                           {0, 0, -(z_far + z_near) / (z_far - z_near),
                            -(2 * z_far * z_near) / (z_far - z_near)},
                           {0, 0, -1, 0}};
    memcpy(out->m, m, sizeof(float[4][4]));
}

void mat4tomat3(mat3 *out, const mat4 *in) {
    out->m[0][0] = in->m[0][0];
    out->m[0][1] = in->m[0][1];
    out->m[0][2] = in->m[0][2];
    out->m[1][0] = in->m[1][0];
    out->m[1][1] = in->m[1][1];
    out->m[1][2] = in->m[1][2];
    out->m[2][0] = in->m[2][0];
    out->m[2][1] = in->m[2][1];
    out->m[2][2] = in->m[2][2];
}

void matinversetranspose(mat3 *out, const mat3 *m) {
    float Determinant =
        +m->m[0][0] * (m->m[1][1] * m->m[2][2] - m->m[1][2] * m->m[2][1]) -
        m->m[0][1] * (m->m[1][0] * m->m[2][2] - m->m[1][2] * m->m[2][0]) +
        m->m[0][2] * (m->m[1][0] * m->m[2][1] - m->m[1][1] * m->m[2][0]);

    /* clang-format off */
    out->m[0][0] = +(m->m[1][1] * m->m[2][2] - m->m[2][1] * m->m[1][2]) / Determinant;
    out->m[0][1] = -(m->m[1][0] * m->m[2][2] - m->m[2][0] * m->m[1][2]) / Determinant;
    out->m[0][2] = +(m->m[1][0] * m->m[2][1] - m->m[2][0] * m->m[1][1]) / Determinant;
    out->m[1][0] = -(m->m[0][1] * m->m[2][2] - m->m[2][1] * m->m[0][2]) / Determinant;
    out->m[1][1] = +(m->m[0][0] * m->m[2][2] - m->m[2][0] * m->m[0][2]) / Determinant;
    out->m[1][2] = -(m->m[0][0] * m->m[2][1] - m->m[2][0] * m->m[0][1]) / Determinant;
    out->m[2][0] = +(m->m[0][1] * m->m[1][2] - m->m[1][1] * m->m[0][2]) / Determinant;
    out->m[2][1] = -(m->m[0][0] * m->m[1][2] - m->m[1][0] * m->m[0][2]) / Determinant;
    out->m[2][2] = +(m->m[0][0] * m->m[1][1] - m->m[1][0] * m->m[0][1]) / Determinant;
    /* clang-format on */
}

void mat4mulv4(vec4 out, const mat4 *m, const vec4 v) {
    /* clang-format off */
    out[0] = v[0] * m->m[0][0] + v[1] * m->m[1][0] + v[2] * m->m[2][0] + v[3] * m->m[3][0];
    out[1] = v[0] * m->m[0][1] + v[1] * m->m[1][1] + v[2] * m->m[2][1] + v[3] * m->m[3][1];
    out[2] = v[0] * m->m[0][2] + v[1] * m->m[1][2] + v[2] * m->m[2][2] + v[3] * m->m[3][2];
    out[3] = v[0] * m->m[0][3] + v[1] * m->m[1][3] + v[2] * m->m[2][3] + v[3] * m->m[3][3];
    /* clang-format on */
}

static void test_mat4mulv4() {
    mat4 m = {.m = {
                  {4, 1, 0, 0},
                  {0, 0, 3, 2},
                  {1, 1, 1, 1},
                  {0, 0, 2, 0},
              }};

    vec4 v = {3, 2, 4, 1};

    vec4 out;
    mat4mulv4(out, &m, v);

    assert(out[0] == 16);
    assert(out[1] == 7);
    assert(out[2] == 12);
    assert(out[3] == 8);
}

void matprint(const mat4 *m) {
    printf("%f %f %f %f\n%f %f %f %f\n%f %f %f %f\n%f %f %f %f\n", m->m[0][0],
           m->m[1][0], m->m[2][0], m->m[3][0], m->m[0][1], m->m[1][1],
           m->m[2][1], m->m[3][1], m->m[0][2], m->m[1][2], m->m[2][2],
           m->m[3][2], m->m[0][3], m->m[1][3], m->m[2][3], m->m[3][3]);
}

static void test_vec() {
    test_vecmag();
    test_veccross();
}

static void test_mat() { test_mat4mulv4(); }

void matvec_test() {
    test_vec();
    test_mat();
}
