#include "SDL_video.h"
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <OpenGL/glew.h>
#else
#include <GL/glew.h>
#endif
#include <SDL.h>

#include "obj.h"

const int DEBUG = 1;
const int VERBOSE = 1;

static float min(float a, float b) { return (a < b) ? a : b; }

static float max(float a, float b) { return (a > b) ? a : b; }

static bool compile_shader(GLenum type, const char *const filename,
                           GLuint *out);

int main(int argc, const char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s [file.obj]\n", argv[0]);
        return 1;
    }

    const char *obj_filename = argv[1];
    FILE *obj_file = fopen(obj_filename, "r");
    if (obj_file == NULL) {
        perror("Unable to open obj file");
        return 1;
    }

    struct obj_obj *obj = obj_read(obj_file);
    if (obj == NULL) {
        fprintf(stderr, "Failed to parse obj file");
        return 1;
    }
    fclose(obj_file);

    if (VERBOSE)
        printf("Loaded '%s' with %d vertices\n", obj_filename, obj->v->n);

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

    GLuint vs;
    compile_shader(GL_VERTEX_SHADER, "shader.vert", &vs);
    glAttachShader(program, vs);

    GLuint fs;
    compile_shader(GL_FRAGMENT_SHADER, "shader.frag", &fs);
    glAttachShader(program, fs);

    glLinkProgram(program);

    GLint program_linked;
    glGetProgramiv(program, GL_LINK_STATUS, &program_linked);
    if (program_linked != GL_TRUE) {
        GLchar message[1024];
        glGetProgramInfoLog(program, 1024, NULL, message);
        fprintf(stderr, "Failed to link shader program: %s\n", message);
    }

    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, obj->v->n * sizeof(struct obj_vertex),
                 obj->v->v, GL_STATIC_DRAW);

    GLuint pos = glGetAttribLocation(program, "pos");
    glEnableVertexAttribArray(pos);
    glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glUseProgram(program);

    glViewport(0, 0, W, H);

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
            case SDL_QUIT:
                running = 0;
                break;
            default:
                break;
            }
        }

        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glDrawArrays(GL_POINTS, 0, obj->v->n);

        SDL_GL_SwapWindow(window);
    }

    obj_free(obj);

    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

static bool compile_shader(GLenum type, const char *const filename,
                           GLuint *out) {
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

    GLuint shader = glCreateShader(type);
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

    *out = shader;
    return true;
}
