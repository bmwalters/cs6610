#include "obj.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

static void vertex_vector_init(struct obj_vertex_vector *vec) {
    vec->n = 0;
    vec->c = 0;
    vec->v = NULL;
}

static void vertex_vector_release(struct obj_vertex_vector *vec) {
    vec->n = 0;
    vec->c = 0;
    free(vec->v);
    vec->v = NULL;
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

void obj_init(struct obj_obj *obj) { vertex_vector_init(&obj->v); }

void obj_release(struct obj_obj *obj) { vertex_vector_release(&obj->v); }

bool obj_read(struct obj_obj *obj, FILE *file) {
    const int nline = 1024;
    char line[nline];

    while (fgets(line, nline, file) != NULL) {
        if (line[0] == 'v' && line[1] == ' ') {
            struct obj_vertex vertex;
            if (sscanf(line, "v %f %f %f", &vertex.x, &vertex.y, &vertex.z) ==
                EOF)
                return false;

            if (!vertex_vector_add(&obj->v, &vertex))
                return false;
        }
    }

    return true;
}
