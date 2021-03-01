#pragma once

#include "mtl.h"
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

struct obj_size_t_vector {
    size_t n;
    size_t c;
    size_t *v;
};

struct obj_obj {
    /* vertices */
    struct obj_vertex_vector v;

    /* texture coords */
    struct obj_vertex_vector t;

    /* normals */
    struct obj_vertex_vector n;

    /* vertex faces */
    struct obj_triface_vector vf;

    /* texture coords */
    struct obj_triface_vector tf;

    /* normal faces */
    struct obj_triface_vector nf;

    /* materials */
    struct mtl_library m;

    /* face index -> material index */
    struct obj_size_t_vector fm;
};

void obj_init(struct obj_obj *obj);

void obj_release(struct obj_obj *obj);

bool obj_read(struct obj_obj *obj, const char *filename);

void obj_bounding_box(const struct obj_obj *obj, struct obj_vertex *outmin,
                      struct obj_vertex *outmax);
