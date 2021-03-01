#include "object.h"
#include "matvec.h"
#include <stdlib.h>
#include <string.h>

void object_init(struct object_object *object) { obj_init(&object->obj); }

static bool
naive_make_triangle_buffer(const struct obj_obj *obj, size_t *out_tri_count,
                           void **out_pos_buffer, size_t *out_pos_buffer_size,
                           void **out_norm_buffer, size_t *out_norm_buffer_size,
                           void **out_texcoord_buffer,
                           size_t *out_texcoord_buffer_size);

bool object_init_vao(struct object_object *object, GLuint program) {
    glGenVertexArrays(1, &object->vao);
    glBindVertexArray(object->vao);

    /*
    GLuint ebuffer;
    glGenBuffers(1, &ebuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, obj.vf.n * sizeof(struct obj_triface),
                 obj.vf.v, GL_STATIC_DRAW);
    */

    if (!naive_make_triangle_buffer(
            &object->obj, &object->triangle_count, &object->buffer_pos,
            &object->buffer_pos_size, &object->buffer_norm,
            &object->buffer_norm_size, &object->buffer_texcoord,
            &object->buffer_texcoord_size))
        return false;

    glGenBuffers(1, &object->vbo_norm);
    glBindBuffer(GL_ARRAY_BUFFER, object->vbo_norm);
    glBufferData(GL_ARRAY_BUFFER, object->buffer_norm_size, object->buffer_norm,
                 GL_STATIC_DRAW);

    GLuint normal = glGetAttribLocation(program, "normal");
    glEnableVertexAttribArray(normal);
    glVertexAttribPointer(normal, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glGenBuffers(1, &object->vbo_pos);
    glBindBuffer(GL_ARRAY_BUFFER, object->vbo_pos);
    glBufferData(GL_ARRAY_BUFFER, object->buffer_pos_size, object->buffer_pos,
                 GL_STATIC_DRAW);

    GLuint pos = glGetAttribLocation(program, "pos");
    glEnableVertexAttribArray(pos);
    glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glGenBuffers(1, &object->vbo_texcoord);
    glBindBuffer(GL_ARRAY_BUFFER, object->vbo_texcoord);
    glBufferData(GL_ARRAY_BUFFER, object->buffer_texcoord_size,
                 object->buffer_texcoord, GL_STATIC_DRAW);

    GLuint texcoord = glGetAttribLocation(program, "texcoord");
    glEnableVertexAttribArray(texcoord);
    glVertexAttribPointer(texcoord, 3, GL_FLOAT, GL_FALSE, 0, 0);

    struct mtl_mtl *used_mtl = &object->obj.m.v[0];

    glGenTextures(1, &object->texture_Ka);
    glBindTexture(GL_TEXTURE_2D, object->texture_Ka);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, used_mtl->map_Ka.w,
                 used_mtl->map_Ka.h, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 used_mtl->map_Ka.v);
    glGenerateMipmap(GL_TEXTURE_2D);

    glGenTextures(1, &object->texture_Kd);
    glBindTexture(GL_TEXTURE_2D, object->texture_Kd);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, used_mtl->map_Kd.w,
                 used_mtl->map_Kd.h, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 used_mtl->map_Kd.v);
    glGenerateMipmap(GL_TEXTURE_2D);

    glGenTextures(1, &object->texture_Ks);
    glBindTexture(GL_TEXTURE_2D, object->texture_Ks);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, used_mtl->map_Ks.w,
                 used_mtl->map_Ks.h, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 used_mtl->map_Ks.v);
    glGenerateMipmap(GL_TEXTURE_2D);

    return true;
}

void object_release(struct object_object *object) {
    obj_release(&object->obj);
    free(object->buffer_norm);
    free(object->buffer_pos);
    free(object->buffer_texcoord);
    glDeleteTextures(1, &object->texture_Ka);
    glDeleteTextures(1, &object->texture_Kd);
    glDeleteTextures(1, &object->texture_Ks);
    glDeleteBuffers(1, &object->vbo_norm);
    glDeleteBuffers(1, &object->vbo_pos);
    glDeleteBuffers(1, &object->vbo_texcoord);
    glDeleteVertexArrays(1, &object->vao);
}

void object_render(const struct object_object *object, GLuint program,
                   const mat4 *projection, const mat4 *view) {
    const struct obj_obj *obj = &object->obj;

    /* bind vao */
    glBindVertexArray(object->vao);

    /* bind textures */
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, object->texture_Ka);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, object->texture_Kd);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, object->texture_Ks);

    /* set texture uniforms */
    glUniform1i(glGetUniformLocation(program, "map_Ka"), 0);
    glUniform1i(glGetUniformLocation(program, "map_Kd"), 1);
    glUniform1i(glGetUniformLocation(program, "map_Ks"), 2);

    /* set other material uniforms */
    glUniform3fv(glGetUniformLocation(program, "Ka"), 1, &obj->m.v[0].Ka[0]);
    glUniform3fv(glGetUniformLocation(program, "Kd"), 1, &obj->m.v[0].Kd[0]);
    glUniform3fv(glGetUniformLocation(program, "Ks"), 1, &obj->m.v[0].Ks[0]);

    glUniform1f(glGetUniformLocation(program, "Ns"), obj->m.v[0].Ns);

    // TODO: Store this inside object and have object_transform(...) which
    // mutates - to skip doing every frame?
    // model -> world transformation "model matrix"
    mat4 obj_trans;
    mattranslate(&obj_trans, 0, 0, 0); // -0.6
    mat4 obj_rot;
    matrotatex(&obj_rot, object->rx);
    mat4 obj_scale;
    matscale(&obj_scale, object->scale);
    mat4 obj_center;
    mattranslate(&obj_center, object->tx, object->ty, object->tz);

    mat4 temp1, temp2, model;
    matmul(&temp2, &obj_trans, &obj_rot);
    matmul(&temp1, &temp2, &obj_scale);
    matmul(&model, &temp1, &obj_center);

    // mvp. v' = P V M v
    mat4 mv, mvp;
    matmul(&mv, view, &model);
    matmul(&mvp, projection, &mv);

    glUniformMatrix4fv(glGetUniformLocation(program, "mv"), 1, GL_FALSE,
                       &mv.m[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(program, "mvp"), 1, GL_FALSE,
                       &mvp.m[0][0]);

    mat3 mv3, mv_normals;
    mat4tomat3(&mv3, &mv);
    matinversetranspose(&mv_normals, &mv3);
    glUniformMatrix3fv(glGetUniformLocation(program, "mv_normals"), 1, GL_FALSE,
                       &mv_normals.m[0][0]);

    glDrawArrays(GL_TRIANGLES, 0, object->triangle_count * 3);
    // glDrawElements(GL_TRIANGLES, obj.vf.n * 3, GL_UNSIGNED_INT, 0);
}

// TODO: replace with glDrawElements etc.
static bool
naive_make_triangle_buffer(const struct obj_obj *obj, size_t *out_tri_count,
                           void **out_pos_buffer, size_t *out_pos_buffer_size,
                           void **out_norm_buffer, size_t *out_norm_buffer_size,
                           void **out_texcoord_buffer,
                           size_t *out_texcoord_buffer_size) {
    if (obj->vf.n != obj->nf.n) {
        fprintf(stderr, "Not sure how to handle obj with #vf != #nf\n");
        return false;
    }

    size_t tri_size = sizeof(float[3][3]);
    size_t n_tris = obj->vf.n;

    float *pos_buffer = calloc(n_tris, tri_size);
    if (pos_buffer == NULL) {
        fprintf(stderr, "Failed to allocate triangle pos buffer\n");
        return false;
    }

    float *norm_buffer = calloc(n_tris, tri_size);
    if (norm_buffer == NULL) {
        fprintf(stderr, "Failed to allocate triangle norm buffer\n");
        return false;
    }

    float *texcoord_buffer = calloc(n_tris, tri_size);
    if (texcoord_buffer == NULL) {
        fprintf(stderr, "Failed to allocate triangle texcoord buffer\n");
        return false;
    }

    for (size_t tri = 0; tri < n_tris; tri++) {
        for (size_t vert = 0; vert < 3; vert++) {
            size_t verti = obj->vf.v[tri].v[vert] - 1;
            memcpy(pos_buffer + tri * 9 + vert * 3, &obj->v.v[verti],
                   sizeof(float[3]));

            size_t nverti = obj->nf.v[tri].v[vert] - 1;
            memcpy(norm_buffer + tri * 9 + vert * 3, &obj->n.v[nverti],
                   sizeof(float[3]));

            size_t texcoordi = obj->tf.v[tri].v[vert] - 1;
            memcpy(texcoord_buffer + tri * 9 + vert * 3, &obj->t.v[texcoordi],
                   sizeof(float[3]));
        }
    }

    *out_tri_count = n_tris;
    *out_pos_buffer = pos_buffer;
    *out_pos_buffer_size = n_tris * tri_size;
    *out_norm_buffer = norm_buffer;
    *out_norm_buffer_size = n_tris * tri_size;
    *out_texcoord_buffer = texcoord_buffer;
    *out_texcoord_buffer_size = n_tris * tri_size;

    return true;
}
