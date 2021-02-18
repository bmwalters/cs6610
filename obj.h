#pragma once

#include <stdbool.h>
#include <stdio.h>

struct obj_vertex {
    float x;
    float y;
    float z;
};

struct obj_triface {
    int v[3];
};

struct obj_vertex_vector {
    size_t n;
    size_t c;
    struct obj_vertex *v;
};

struct obj_triface_vector {
    size_t n;
    size_t c;
    struct obj_triface *v;
};

struct obj_obj {
    /* vertices */
    struct obj_vertex_vector v;

    /* vertex faces */
    struct obj_triface_vector vf;

    /* texture faces */
    struct obj_triface_vector tf;

    /* normal faces */
    struct obj_triface_vector nf;
};

void obj_init(struct obj_obj *obj);

void obj_release(struct obj_obj *obj);

bool obj_read(struct obj_obj *obj, FILE *file);
