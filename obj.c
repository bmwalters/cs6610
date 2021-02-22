#include "obj.h"
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/**
 * obj_vertex_vector
 */

static void obj_vertex_vector_init(struct obj_vertex_vector *vec) {
    vec->n = 0;
    vec->c = 0;
    vec->v = NULL;
}

static void obj_vertex_vector_release(struct obj_vertex_vector *vec) {
    vec->n = 0;
    vec->c = 0;
    free(vec->v);
    vec->v = NULL;
}

static bool obj_vertex_vector_add(struct obj_vertex_vector *vec,
                                  struct obj_vertex *item) {
    while (vec->n >= vec->c) {
        unsigned int new_c = (vec->c == 0) ? 1 : vec->c * 2;
        struct obj_vertex *new_v = calloc(new_c, sizeof(struct obj_vertex));
        if (new_v == NULL)
            return false;

        vec->v = memcpy(new_v, vec->v, sizeof(struct obj_vertex) * vec->n);
        vec->c = new_c;
    }

    vec->v[vec->n] = *item;
    vec->n++;

    return true;
}

/**
 * obj_triface_vector
 */

static void obj_triface_vector_init(struct obj_triface_vector *vec) {
    vec->n = 0;
    vec->c = 0;
    vec->v = NULL;
}

static void obj_triface_vector_release(struct obj_triface_vector *vec) {
    vec->n = 0;
    vec->c = 0;
    free(vec->v);
    vec->v = NULL;
}

static bool obj_triface_vector_add(struct obj_triface_vector *vec,
                                   struct obj_triface *item) {
    while (vec->n >= vec->c) {
        unsigned int new_c = (vec->c == 0) ? 1 : vec->c * 2;
        struct obj_triface *new_v = calloc(new_c, sizeof(struct obj_triface));
        if (new_v == NULL)
            return false;

        vec->v = memcpy(new_v, vec->v, sizeof(struct obj_triface) * vec->n);
        vec->c = new_c;
    }

    vec->v[vec->n] = *item;
    vec->n++;

    return true;
}

/**
 * obj parsing
 */

void obj_init(struct obj_obj *obj) {
    obj_vertex_vector_init(&obj->v);
    obj_vertex_vector_init(&obj->t);
    obj_vertex_vector_init(&obj->n);
    obj_triface_vector_init(&obj->vf);
    obj_triface_vector_init(&obj->tf);
    obj_triface_vector_init(&obj->nf);
}

void obj_release(struct obj_obj *obj) {
    obj_vertex_vector_release(&obj->v);
    obj_vertex_vector_release(&obj->t);
    obj_vertex_vector_release(&obj->n);
    obj_triface_vector_release(&obj->vf);
    obj_triface_vector_release(&obj->tf);
    obj_triface_vector_release(&obj->nf);
}

static bool parse_face_line();

bool obj_read(struct obj_obj *obj, FILE *file) {
    const int nline = 1024;
    char line[nline];

    while (fgets(line, nline, file) != NULL) {
        if (line[0] == 'v' && line[1] == ' ') {
            struct obj_vertex vertex;
            if (sscanf(line, "v %f %f %f", &vertex.x, &vertex.y, &vertex.z) ==
                EOF)
                return false;

            if (!obj_vertex_vector_add(&obj->v, &vertex))
                return false;
        } else if (line[0] == 'v' && line[1] == 'n' && line[2] == ' ') {
            struct obj_vertex normal;
            if (sscanf(line, "vn %f %f %f", &normal.x, &normal.y, &normal.z) ==
                EOF)
                return false;

            if (!obj_vertex_vector_add(&obj->n, &normal))
                return false;
        } else if (line[0] == 'v' && line[1] == 't' && line[2] == ' ') {
            struct obj_vertex vertex;
            if (sscanf(line, "vt %f %f %f", &vertex.x, &vertex.y, &vertex.z) ==
                EOF)
                return false;

            if (!obj_vertex_vector_add(&obj->t, &vertex))
                return false;
        } else if (line[0] == 'f' && line[1] == ' ') {
            if (!parse_face_line(obj, line))
                return false;
        } else if (strncmp(line, "mtllib ", 7) == 0) {
            // TODO
        } else if (strncmp(line, "usemtl ", 7) == 0) {
            // TODO
        }
    }

    return true;
}

static bool parse_face_line(struct obj_obj *obj, const char *line) {
    /* [vertex, texture, normal] */
    struct obj_triface faces_by_kind[3];
    const char *c = line + 2;

    int kinds_read = 0;
    int elems_read = 0;

    bool has_textures = false;
    bool has_normals = false;

    bool read_number_last = false;

    while (true) {
        switch (*c) {
        case '/':
            assert(kinds_read < 3);
            if (!read_number_last) {
                assert(!(kinds_read == 1 && has_textures));
                assert(!(kinds_read == 2 && has_normals));
                kinds_read++;
            }
            read_number_last = false;
            c++;
            break;
        case ' ':
        case '\t':
        case '\r':
        case '\n':
        case 0:
        case '#': {
            bool should_terminate = *c == 0 || *c == '#';

            if (kinds_read > 0) {
                assert(!(has_textures && kinds_read < 2));
                assert(!(has_normals && kinds_read < 3));

                elems_read++;
                kinds_read = 0;
                if (elems_read == 3) {
                    if (!obj_triface_vector_add(&obj->vf, &faces_by_kind[0]))
                        return false;
                    if (has_textures &&
                        !obj_triface_vector_add(&obj->tf, &faces_by_kind[1]))
                        return false;
                    if (has_normals &&
                        !obj_triface_vector_add(&obj->nf, &faces_by_kind[2]))
                        return false;

                    for (int i = 0; i < 3; i++)
                        faces_by_kind[i].v[1] = faces_by_kind[i].v[2];

                    elems_read = 2;
                }
            }

            if (should_terminate)
                return true;
            read_number_last = false;
            c++;
            break;
        }
        default: {
            assert(*c >= '0' && *c <= '9');
            int val, read;
            int got = sscanf(c, "%d%n", &val, &read);
            assert(got != EOF);

            // TODO: support negative references
            assert(val > 0);

            assert(kinds_read < 3);
            faces_by_kind[kinds_read].v[elems_read] = val;
            read_number_last = true;
            kinds_read++;

            if (elems_read == 0 && kinds_read == 2)
                has_textures = true;
            else if (elems_read == 0 && kinds_read == 3)
                has_normals = true;

            c += read;
            break;
        }
        }
    }
}
