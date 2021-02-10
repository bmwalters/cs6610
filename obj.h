#pragma once

#include <stdio.h>

struct obj_vertex {
    float x;
    float y;
    float z;
};

struct obj_vertex_vector {
    unsigned int n;
    unsigned int c;
    struct obj_vertex* v;
};

struct obj_obj {
    /* vertices */
    struct obj_vertex_vector* v;
};

struct obj_obj* obj_read(FILE* file);
void obj_free(struct obj_obj* obj);
