#pragma once

#include "gl.h"
#include "matvec.h"
#include "obj.h"

struct object_object {
    struct obj_obj obj;
    float tx;
    float ty;
    float tz;
    float rx;
    float scale;
    size_t triangle_count;
    void *buffer_pos;
    size_t buffer_pos_size;
    void *buffer_norm;
    size_t buffer_norm_size;
    void *buffer_texcoord;
    size_t buffer_texcoord_size;
    GLuint vao;
    GLuint vbo_pos;
    GLuint vbo_norm;
    GLuint vbo_texcoord;
    GLuint texture_Ka;
    GLuint texture_Kd;
    GLuint texture_Ks;
};

void object_init(struct object_object *object);

bool object_init_vao(struct object_object *object, GLuint program);

void object_release(struct object_object *object);

void object_render(const struct object_object *object, GLuint program,
                   const mat4 *projection, const mat4 *view);
