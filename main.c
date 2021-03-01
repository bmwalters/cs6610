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

    GLuint program = glCreateProgram();
    compile_shader_program(program);

    // TODO: Build system so locations from program don't change after
    // shader recompilation.
    if (!object_init_vao(&teapot, program)) {
        fprintf(stderr, "Could not initialize object buffers\n");
        return 1;
    }

    // TODO: call this after recompilation?
    glUseProgram(program);

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
                        yaw += event.motion.xrel * 0.005;
                        pitch += event.motion.yrel * 0.005;
                    }
                } else if (event.motion.state & SDL_BUTTON_RMASK) {
                    cam_dist += event.motion.yrel * 0.005;
                }
                break;
            }
            case SDL_MOUSEWHEEL:
                fovy = fovy - event.wheel.y * 0.02;
                fovy = fmin(M_PI / 4, fmax(M_PI / 32, fovy));
                break;
            case SDL_QUIT:
                running = 0;
                break;
            default:
                break;
            }
        }

        // view -> clip space transformation
        // "projection matrix"
        mat4 projection;
        matscale(&projection, 1);
        matperspective(&projection, fovy, (float)W / (float)H, z_near, z_far);

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

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glEnable(GL_DEPTH_TEST);
        glViewport(0, 0, W, H);
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        object_render(&teapot, program, &projection, &view);
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
