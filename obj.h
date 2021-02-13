#pragma once

#include <stdbool.h>
#include <stdio.h>

struct obj_vertex {
    float x;
    float y;
    float z;
};

struct obj_vertex_vector {
    unsigned int n;
    unsigned int c;
    struct obj_vertex *v;
};

struct obj_obj {
    /* vertices */
    struct obj_vertex_vector v;
};

void obj_init(struct obj_obj *obj);

void obj_release(struct obj_obj *obj);

bool obj_read(struct obj_obj *obj, FILE *file);
