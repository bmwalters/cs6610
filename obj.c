#include "obj.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

static struct obj_vertex_vector *vertex_vector_new() {
    struct obj_vertex_vector *vec = malloc(sizeof(struct obj_vertex_vector));
    if (vec == NULL)
        return NULL;

    vec->n = 0;
    vec->c = 0;
    vec->v = NULL;

    return vec;
}

static bool vertex_vector_add(struct obj_vertex_vector *vec,
                              struct obj_vertex *item) {
    while (vec->n >= vec->c) {
        unsigned int new_c = (vec->c == 0) ? 1 : vec->c * 2;
        struct obj_vertex *new_v =
            calloc(new_c, sizeof(struct obj_vertex_vector));
        if (new_v == NULL)
            return false;

        vec->v =
            memcpy(new_v, vec->v, sizeof(struct obj_vertex_vector) * vec->n);
        vec->c = new_c;
    }

    vec->v[vec->n] = *item;
    vec->n++;

    return true;
}

static void vertex_vector_free(struct obj_vertex_vector *vec) {
    vec->n = 0;
    vec->c = 0;
    free(vec->v);
    vec->v = NULL;
}

struct obj_obj *obj_read(FILE *file) {
    struct obj_obj *obj = malloc(sizeof(struct obj_obj));
    if (obj == NULL)
        return NULL;

    struct obj_vertex_vector *vertices = vertex_vector_new();
    if (vertices == NULL)
        return NULL;

    const int nline = 1024;
    char line[nline];

    while (fgets(line, nline, file) != NULL) {
        if (line[0] == 'v' && line[1] == ' ') {
            struct obj_vertex vertex;
            if (sscanf(line, "v %f %f %f", &vertex.x, &vertex.y, &vertex.z) ==
                EOF) {
                vertex_vector_free(vertices);
                return NULL;
            }

            if (!vertex_vector_add(vertices, &vertex)) {
                vertex_vector_free(vertices);
                return NULL;
            }
        }
    }

    obj->v = vertices;

    return obj;
}

void obj_free(struct obj_obj *obj) {
    vertex_vector_free(obj->v);
    obj->v = NULL;
}
