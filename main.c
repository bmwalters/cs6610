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

struct camera {
    const float z_near;
    const float z_far;
    float fovy;
    float yaw;
    float pitch;
    float dist;
};

static void camera_build_matrices(const struct camera *cam,
                                  const unsigned int W, const unsigned int H,
                                  mat4 *view, mat4 *projection) {
    matperspective(projection, cam->fovy, (float)W / (float)H, cam->z_near,
                   cam->z_far);
    vec3 eye;
    veceulerangles(eye, cam->yaw, cam->pitch);
    vecscale(eye, cam->dist);
    vec3 target = {0, 0, 0};
    vec3 up = {0, 1, 0};
    matview(view, eye, target, up);
}

struct cube {
    GLuint program;
    GLuint cubemap;
    GLuint vao;
    GLuint vbo;
    GLsizei nvert;
};

static void cube_init(struct cube *cube);
static void cube_release(struct cube *cube);
static void cube_render(const struct cube *cube, const mat4 *PV);

static bool compile_shader_file(const char *const filename, GLuint shader);
static bool compile_shader_program(GLuint program, const char *filename_vert,
                                   const char *filename_frag);

static void testmain() { matvec_test(); }

int main(int argc, const char *argv[]) {
    if (DEBUG)
        testmain();

    if (argc < 2) {
        fprintf(stderr, "Usage: %s [file.obj]\n", argv[0]);
        return 1;
    }

    // TODO: Load teapot.

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

    struct camera cam = {
        .z_near = 0.1,
        .z_far = 100,
        .fovy = M_PI / 4,
        .yaw = M_PI / 3,
        .pitch = M_PI / 6,
        .dist = 4,
    };

    struct cube cube;
    cube_init(&cube);

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
                default:
                    break;
                }
                break;
            case SDL_MOUSEMOTION: {
                if (event.motion.state & SDL_BUTTON_LMASK) {
                    cam.yaw += event.motion.xrel * 0.005;
                    cam.pitch += event.motion.yrel * 0.005;
                } else if (event.motion.state & SDL_BUTTON_RMASK) {
                    cam.dist += event.motion.yrel * 0.005;
                }
                break;
            }
            case SDL_MOUSEWHEEL:
                cam.fovy = cam.fovy - event.wheel.y * 0.02;
                cam.fovy = fmin(M_PI / 4, fmax(M_PI / 32, cam.fovy));
                break;
            case SDL_QUIT:
                running = 0;
                break;
            default:
                break;
            }
        }

        mat4 view, projection;
        camera_build_matrices(&cam, W, H, &view, &projection);

        mat4 PV;
        matmul(&PV, &projection, &view);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
        glViewport(0, 0, W, H);
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        cube_render(&cube, &PV);

        SDL_GL_SwapWindow(window);
    }

    cube_release(&cube);

    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

static void cube_init(struct cube *cube) {
    // -------------------------------------------------------------------------
    // Load Textures

    struct mtl_texture_image images[6];
    // TODO: handle errors
    mtl_texture_image_load(&images[0], "cubemap/cubemap_posx.png");
    mtl_texture_image_load(&images[1], "cubemap/cubemap_negx.png");
    mtl_texture_image_load(&images[2], "cubemap/cubemap_posy.png");
    mtl_texture_image_load(&images[3], "cubemap/cubemap_negy.png");
    mtl_texture_image_load(&images[4], "cubemap/cubemap_posz.png");
    mtl_texture_image_load(&images[5], "cubemap/cubemap_negz.png");

    glGenTextures(1, &cube->cubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cube->cubemap);

    for (int i = 0; i < 6; i++) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA,
                     images[i].w, images[i].h, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                     images[i].v);
        mtl_texture_image_release(&images[i]);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    // -------------------------------------------------------------------------
    // Load Shaders

    cube->program = glCreateProgram();
    compile_shader_program(cube->program, "cubemap.vert", "cubemap.frag");

    // -------------------------------------------------------------------------
    // Load Geometry

    enum { cube_geometry_nvert = 6 * 2 * 3 };
    enum { cube_geometry_nent = 6 * 6 * 3 };
    float cube_geometry[cube_geometry_nent] = {
        -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f,
        1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f,

        -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f,
        -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,

        1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f,

        -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,

        -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f,

        -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f,
        1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f};
    cube->nvert = cube_geometry_nvert;

    glGenVertexArrays(1, &cube->vao);
    glBindVertexArray(cube->vao);

    glGenBuffers(1, &cube->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, cube->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_geometry), cube_geometry,
                 GL_STATIC_DRAW);

    GLuint pos_loc = glGetAttribLocation(cube->program, "pos");
    glEnableVertexAttribArray(pos_loc);
    glVertexAttribPointer(pos_loc, 3, GL_FLOAT, GL_FALSE, 0, 0);
}

static void cube_release(struct cube *cube) {
    glDeleteBuffers(1, &cube->vbo);
    glDeleteVertexArrays(1, &cube->vao);
    glDeleteTextures(1, &cube->cubemap);
    glDeleteProgram(cube->program);
}

static void cube_render(const struct cube *cube, const mat4 *PV) {
    glUseProgram(cube->program);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cube->cubemap);
    glUniform1i(glGetUniformLocation(cube->program, "env"), 0);

    glUniformMatrix4fv(glGetUniformLocation(cube->program, "vp"), 1, GL_FALSE,
                       &PV->m[0][0]);

    glBindVertexArray(cube->vao);

    glDrawArrays(GL_TRIANGLES, 0, cube->nvert);
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

static bool compile_shader_program(GLuint program, const char *filename_vert,
                                   const char *filename_frag) {
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    compile_shader_file(filename_vert, vs);
    glAttachShader(program, vs);

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    compile_shader_file(filename_frag, fs);
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
