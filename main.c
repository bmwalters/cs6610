#include "gl.h"
#include "matvec.h"
#include "obj.h"
#include "object.h"
#include <SDL.h>
#include <SDL_video.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <tgmath.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

const int DEBUG = 1;
const int VERBOSE = 1;

static bool compile_shader_file(const char *const filename, GLuint shader);
static bool compile_shader_program(GLuint program);

static void testmain() { matvec_test(); }

int main(int argc, const char *argv[]) {
    if (DEBUG)
        testmain();

    if (argc < 2) {
        fprintf(stderr, "Usage: %s [file.obj]\n", argv[0]);
        return 1;
    }

    struct object_object teapot;
    object_init(&teapot);
    struct obj_obj *obj = &teapot.obj;

    const char *obj_filename = argv[1];
    if (!obj_read(obj, obj_filename)) {
        fprintf(stderr, "Failed to parse obj file\n");
        return 1;
    }

    if (VERBOSE)
        printf("Loaded '%s'.\n\t%zu geometric vertices\n\t%zu texture "
               "vertices\n\t%zu vertex normals\n\t%zu faces\n\t%zu normal "
               "faces\n\t%zu texture faces\n\t%zu materials\n\t%zu vertices "
               "with materials\n",
               obj_filename, obj->v.n, obj->t.n, obj->n.n, obj->vf.n, obj->nf.n,
               obj->tf.n, obj->m.n, obj->fm.n);

    if (VERBOSE && obj->m.n > 0)
        printf("Material 0:\n\tKa %f %f %f\n", obj->m.v[0].Ka[0],
               obj->m.v[0].Ka[1], obj->m.v[0].Ka[2]);

    SDL_Init(SDL_INIT_VIDEO);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_CORE);

    if (DEBUG)
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);

    if (SDL_GL_SetSwapInterval(-1) < 0)
        SDL_GL_SetSwapInterval(1);

    const int W = 640;
    const int H = 480;

    SDL_Window *window = SDL_CreateWindow(
        "Hello World", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, W, H,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);

    if (window == NULL) {
        fprintf(stderr, "Could not create window: %s\n", SDL_GetError());
        return 1;
    }

    SDL_GLContext context = SDL_GL_CreateContext(window);

    if (VERBOSE) {
        printf("GL_VERSION: %s\nGL_EXTENSIONS: %s\nGL_VENDOR: %s\nGL_RENDERER: "
               "%s\n",
               glGetString(GL_VERSION), glGetString(GL_EXTENSIONS),
               glGetString(GL_VENDOR), glGetString(GL_RENDERER));
    }

    GLenum glewErr;
    if ((glewErr = glewInit()) != GLEW_OK) {
        fprintf(stderr, "glewInit failed: %s\n", glewGetErrorString(glewErr));
        return 1;
    }

    if (VERBOSE)
        printf("GLEW_VERSION: %s\n", glewGetString(GLEW_VERSION));

    /* create frame buffer */
    GLuint framebuffer;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    /* create texture */
    const unsigned int texw = 512;
    const unsigned int texh = 512;
    GLuint rendered_texture;
    glGenTextures(1, &rendered_texture);
    glBindTexture(GL_TEXTURE_2D, rendered_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texw, texh, 0, GL_RGB,
                 GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    /* create depth buffer */
    GLuint depth_buffer;
    glGenRenderbuffers(1, &depth_buffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depth_buffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, texw, texh);

    /* configure frame buffer */
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, depth_buffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           rendered_texture, 0);
    GLenum draw_buffers[1] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, draw_buffers);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "framebuffer status != complete\n");
        return 1;
    }

    GLuint program = glCreateProgram();
    compile_shader_program(program);

    // TODO: Build system so locations from program don't change after
    // shader recompilation.
    if (!object_init_vao(&teapot, program)) {
        fprintf(stderr, "Could not initialize object buffers\n");
        return 1;
    }

    float texturedquad_buffer_pos[18] = {-1, -1, 0, -1, 1, 0, 1,  -1, 0,
                                         1,  -1, 0, 1,  1, 0, -1, 1,  0};
    float texturedquad_buffer_norm[18] = {0, 0, 1, 0, 0, 1, 0, 0, 1,
                                          0, 0, 1, 0, 0, 1, 0, 0, 1};
    float texturedquad_buffer_texcoord[18] = {0, 0, 0, 0, 1, 0, 1, 0, 0,
                                              1, 0, 0, 1, 1, 1, 0, 1, 0};

    GLuint texturedquad_vao;
    glGenVertexArrays(1, &texturedquad_vao);
    glBindVertexArray(texturedquad_vao);

    GLuint texturedquad_vbo_norm;
    glGenBuffers(1, &texturedquad_vbo_norm);
    glBindBuffer(GL_ARRAY_BUFFER, texturedquad_vbo_norm);
    glBufferData(GL_ARRAY_BUFFER, 18 * sizeof(float), texturedquad_buffer_norm,
                 GL_STATIC_DRAW);

    GLuint normal_loc = glGetAttribLocation(program, "normal");
    glEnableVertexAttribArray(normal_loc);
    glVertexAttribPointer(normal_loc, 3, GL_FLOAT, GL_FALSE, 0, 0);

    GLuint texturedquad_vbo_pos;
    glGenBuffers(1, &texturedquad_vbo_pos);
    glBindBuffer(GL_ARRAY_BUFFER, texturedquad_vbo_pos);
    glBufferData(GL_ARRAY_BUFFER, 18 * sizeof(float), texturedquad_buffer_pos,
                 GL_STATIC_DRAW);

    GLuint pos_loc = glGetAttribLocation(program, "pos");
    glEnableVertexAttribArray(pos_loc);
    glVertexAttribPointer(pos_loc, 3, GL_FLOAT, GL_FALSE, 0, 0);

    GLuint texturedquad_vbo_texcoord;
    glGenBuffers(1, &texturedquad_vbo_texcoord);
    glBindBuffer(GL_ARRAY_BUFFER, texturedquad_vbo_texcoord);
    glBufferData(GL_ARRAY_BUFFER, 18 * sizeof(float),
                 texturedquad_buffer_texcoord, GL_STATIC_DRAW);

    GLuint texcoord_loc = glGetAttribLocation(program, "texcoord");
    glEnableVertexAttribArray(texcoord_loc);
    glVertexAttribPointer(texcoord_loc, 3, GL_FLOAT, GL_FALSE, 0, 0);

    GLuint texture_white;
    glGenTextures(1, &texture_white);
    glBindTexture(GL_TEXTURE_2D, texture_white);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    unsigned char texture_data_white[4] = {255, 255, 255, 255};
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 &texture_data_white[0]);

    struct obj_vertex obj_min, obj_max;
    obj_bounding_box(obj, &obj_min, &obj_max);

    if (VERBOSE)
        printf("obj bounding box: min=(%f, %f, %f); max=(%f, %f, %f)\n",
               obj_min.x, obj_min.y, obj_min.z, obj_max.x, obj_max.y,
               obj_max.z);

    float obj_fill_scale =
        1 / fmin(fmin(obj_max.x - obj_min.x, obj_max.y - obj_min.y),
                 obj_max.z - obj_min.z);

    if (VERBOSE)
        printf("obj_fill_scale = %f\n", obj_fill_scale);

    float obj_center_xoff = -(fabs(obj_max.x) - fabs(obj_min.x)) / 2;
    float obj_center_yoff = -(fabs(obj_max.y) - fabs(obj_min.y)) / 2;
    float obj_center_zoff = -(fabs(obj_max.z) - fabs(obj_min.z)) / 2;

    const float z_near = 1; // TODO: Fix bugs when z_near = 0.1
    const float z_far = 100;

    float fovy = M_PI / 4;

    float yaw = M_PI / 2;
    float pitch = 0;

    float cam_dist = 1;

    float light_yaw = M_PI / 4;
    float light_pitch = M_PI / 8;
    float light_dist = 3;

    float fovy_outer = M_PI / 4;
    float yaw_outer = M_PI / 2;
    float pitch_outer = 0;
    float cam_dist_outer = 1;

    float light_outer_yaw = M_PI / 4;
    float light_outer_pitch = M_PI / 8;
    float light_outer_dist = 3;

    const unsigned char *key_state = SDL_GetKeyboardState(NULL);

    int running = 1;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                case SDLK_ESCAPE:
                    running = 0;
                    break;
                case SDLK_F6:
                    compile_shader_program(program);
                    break;
                default:
                    break;
                }
                break;
            case SDL_MOUSEMOTION: {
                if (event.motion.state & SDL_BUTTON_LMASK) {
                    if (key_state[SDL_SCANCODE_LCTRL]) {
                        light_yaw += event.motion.xrel * 0.005;
                        light_pitch += event.motion.yrel * 0.005;
                    } else {
                        if (key_state[SDL_SCANCODE_LALT]) {
                            yaw_outer += event.motion.xrel * 0.005;
                            pitch_outer += event.motion.yrel * 0.005;
                        } else {
                            yaw += event.motion.xrel * 0.005;
                            pitch += event.motion.yrel * 0.005;
                        }
                    }
                } else if (event.motion.state & SDL_BUTTON_RMASK) {
                    if (key_state[SDL_SCANCODE_LALT]) {
                        cam_dist_outer += event.motion.yrel * 0.005;
                    } else {
                        cam_dist += event.motion.yrel * 0.005;
                    }
                }
                break;
            }
            case SDL_MOUSEWHEEL:
                if (key_state[SDL_SCANCODE_LALT]) {
                    fovy_outer = fovy_outer - event.wheel.y * 0.02;
                    fovy_outer = fmin(M_PI / 4, fmax(M_PI / 32, fovy_outer));
                } else {
                    fovy = fovy - event.wheel.y * 0.02;
                    fovy = fmin(M_PI / 4, fmax(M_PI / 32, fovy));
                }
                break;
            case SDL_QUIT:
                running = 0;
                break;
            default:
                break;
            }
        }

        // TODO: don't call this every frame?
        glUseProgram(program);

        // view -> clip space transformation
        // "projection matrix"
        mat4 projection;
        matscale(&projection, 1);
        matperspective(&projection, fovy, (float)texw / (float)texh, z_near,
                       z_far);

        // world -> view transformation
        // "view matrix"
        mat4 view;
        vec3 zero = {0, 0, 0};
        vec3 up = {0, 1, 0};
        vec3 eye;
        veceulerangles(eye, yaw, pitch);
        vecscale(eye, cam_dist);
        matview(&view, eye, zero, up);

        glUniform1f(glGetUniformLocation(program, "ambient_intensity"), 0.2);

        vec3 light_pos;
        veceulerangles(light_pos, light_yaw, light_pitch);
        vecscale(light_pos, light_dist);
        vec4 light_pos_h = {light_pos[0], light_pos[1], light_pos[2], 1};
        vec4 light_pos_view_h;
        mat4mulv4(light_pos_view_h, &view, light_pos_h);
        vecscale(light_pos_view_h, 1 / light_pos_view_h[3]);
        vec3 light_pos_view = {light_pos_view_h[0], light_pos_view_h[1],
                               light_pos_view_h[2]};
        glUniform3fv(glGetUniformLocation(program, "light_pos_view"), 1,
                     &light_pos_view[0]);
        glUniform1f(glGetUniformLocation(program, "light_intensity"), 0.8);

        teapot.rx = M_PI / 2;
        teapot.tx = obj_center_xoff;
        teapot.ty = obj_center_yoff;
        teapot.tz = obj_center_zoff;
        teapot.scale = obj_fill_scale;

        /**
         * Render the teapot
         */

        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glEnable(GL_DEPTH_TEST);
        glViewport(0, 0, texw, texh);
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        object_render(&teapot, program, &projection, &view);
        glBindTexture(GL_TEXTURE_2D, rendered_texture);
        glGenerateMipmap(GL_TEXTURE_2D);

        /**
         * Render the textured quad
         */

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glEnable(GL_DEPTH_TEST);
        glViewport(0, 0, W, H);
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        /* bind vao */
        glBindVertexArray(texturedquad_vao);

        /* bind textures */
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture_white);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, rendered_texture);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, rendered_texture);

        /* set texture uniforms */
        glUniform1i(glGetUniformLocation(program, "map_Ka"), 0);
        glUniform1i(glGetUniformLocation(program, "map_Kd"), 1);
        glUniform1i(glGetUniformLocation(program, "map_Ks"), 2);

        /* set other material uniforms */
        vec3 black = {0, 0, 0};
        vec3 gray = {0.124, 0.124, 0.124};
        vec3 white = {1, 1, 1};
        glUniform3fv(glGetUniformLocation(program, "Ka"), 1, gray);
        glUniform3fv(glGetUniformLocation(program, "Kd"), 1, white);
        glUniform3fv(glGetUniformLocation(program, "Ks"), 1, black);

        glUniform1f(glGetUniformLocation(program, "Ns"), 0);

        glUniform1f(glGetUniformLocation(program, "ambient_intensity"), 1);

        vec3 light_outer_pos;
        veceulerangles(light_outer_pos, light_outer_yaw, light_outer_pitch);
        vecscale(light_outer_pos, light_outer_dist);
        vec4 light_outer_pos_h = {light_outer_pos[0], light_outer_pos[1],
                                  light_outer_pos[2], 1};
        vec4 light_outer_pos_view_h;
        mat4mulv4(light_outer_pos_view_h, &view, light_outer_pos_h);
        vecscale(light_outer_pos_view_h, 1 / light_outer_pos_view_h[3]);
        vec3 light_outer_pos_view = {light_outer_pos_view_h[0],
                                     light_outer_pos_view_h[1],
                                     light_outer_pos_view_h[2]};
        glUniform3fv(glGetUniformLocation(program, "light_pos_view"), 1,
                     &light_outer_pos_view[0]);
        glUniform1f(glGetUniformLocation(program, "light_intensity"), 0.8);

        /* set model-view-projection uniforms */
        // view -> clip space transformation
        // "projection matrix"
        mat4 projection_outer;
        matperspective(&projection_outer, fovy_outer, (float)W / (float)H,
                       z_near, z_far);

        // world -> view transformation
        // "view matrix"
        mat4 view_outer;
        vec3 eye_outer;
        veceulerangles(eye_outer, yaw_outer, pitch_outer);
        vecscale(eye_outer, cam_dist_outer);
        matview(&view_outer, eye_outer, zero, up);

        mat4 tq_rot_x;
        matrotatex(&tq_rot_x, M_PI / 4);
        mat4 tq_rot_y;
        matrotatey(&tq_rot_y, M_PI / 4);
        mat4 tq_rot_z;
        matrotatex(&tq_rot_z, M_PI / 4);
        mat4 tq_scale;
        matscale(&tq_scale, 1);
        mat4 tq_center;
        mattranslate(&tq_center, 0, 0, 0);

        mat4 temp1, temp2, tq_model;
        matmul(&temp2, &tq_rot_z, &tq_rot_y);
        matmul(&temp1, &tq_rot_x, &tq_scale);
        matmul(&tq_model, &temp1, &tq_center);

        // mvp. v' = P V M v
        mat4 tq_mv, tq_mvp;
        matmul(&tq_mv, &view_outer, &tq_model);
        matmul(&tq_mvp, &projection_outer, &tq_mv);

        mat3 tq_mv3, tq_mv_normals;
        mat4tomat3(&tq_mv3, &tq_mv);
        matinversetranspose(&tq_mv_normals, &tq_mv3);

        glUniformMatrix4fv(glGetUniformLocation(program, "mv"), 1, GL_FALSE,
                           &tq_mv.m[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(program, "mvp"), 1, GL_FALSE,
                           &tq_mvp.m[0][0]);
        glUniformMatrix3fv(glGetUniformLocation(program, "mv_normals"), 1,
                           GL_FALSE, &tq_mv_normals.m[0][0]);

        /* actually draw the quad */
        glDrawArrays(GL_TRIANGLES, 0, 6);

        SDL_GL_SwapWindow(window);
    }

    object_release(&teapot);

    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

static bool compile_shader_file(const char *const filename, GLuint shader) {
    char shader_source[2048];
    FILE *shader_file = fopen(filename, "r");
    if (shader_file == NULL) {
        fprintf(stderr, "Failed to open %s\n", filename);
        return false;
    }
    GLint shader_len = fread(shader_source, 1, 2048, shader_file);
    if (ferror(shader_file)) {
        fclose(shader_file);
        fprintf(stderr, "Failed to read %s\n", filename);
        return false;
    }
    if (!feof(shader_file)) {
        fclose(shader_file);
        fprintf(stderr, "%s source file too large\n", filename);
        return false;
    }
    fclose(shader_file);

    const GLchar *shader_source_ptr = shader_source;
    glShaderSource(shader, 1, &shader_source_ptr, &shader_len);
    glCompileShader(shader);

    GLint compiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled != GL_TRUE) {
        GLchar message[1024];
        glGetShaderInfoLog(shader, 1024, NULL, message);
        fprintf(stderr, "Failed to link shader program: %s\n", message);
    }

    return true;
}

static bool compile_shader_program(GLuint program) {
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    compile_shader_file("shader.vert", vs);
    glAttachShader(program, vs);

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    compile_shader_file("shader.frag", fs);
    glAttachShader(program, fs);

    glLinkProgram(program);

    GLint program_linked;
    glGetProgramiv(program, GL_LINK_STATUS, &program_linked);
    if (program_linked != GL_TRUE) {
        GLchar message[1024];
        glGetProgramInfoLog(program, 1024, NULL, message);
        fprintf(stderr, "Failed to link shader program: %s\n", message);
    }

    glDetachShader(program, vs);
    glDetachShader(program, fs);

    glDeleteShader(vs);
    glDeleteShader(fs);

    return program_linked == GL_TRUE;
}
