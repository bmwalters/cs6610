#pragma once

typedef struct {
    float m[3][3];
} mat3;
typedef struct {
    float m[4][4];
} mat4;

typedef float vec3[3];
typedef float vec4[4];

/** outm may not alias lm or rm. */
void matmul(mat4 *out, const mat4 *lm, const mat4 *rm);

void matscale(mat4 *out, float scale);

void mattranslate(mat4 *out, float tx, float ty, float tz);

void matrotatex(mat4 *out, float theta);

void matrotatey(mat4 *out, float theta);

void matrotatez(mat4 *out, float theta);

float vecmag(const vec3 v);

void vecnormalize(vec3 v);

void vecscale(vec3 v, float scale);

float vecdot(const vec3 u, const vec3 v);

/** out may not alias u or v. */
void veccross(vec3 out, const vec3 u, const vec3 v);

void matview(mat4 *out, const vec3 eye, const vec3 target, const vec3 up);

void veceulerangles(vec3 out, float yaw, float pitch);

void matperspective(mat4 *out, float fovy, float aspect, float z_near,
                    float z_far);

/** out may not alias in. */
void mat4tomat3(mat3 *out, const mat4 *in);

/** out may not alias m. */
void matinversetranspose(mat3 *out, const mat3 *in);

/** out may not alias m or v. */
void mat4mulv4(vec4 out, const mat4 *m, const vec4 in);

void matprint(const mat4 *m);

void matvec_test();
