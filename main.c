#include "SDL_video.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
#include <SDL.h>

#include "obj.h"

const int DEBUG   = 1;
const int VERBOSE = 1;

static float min(float a, float b) {
    return (a < b) ? a : b;
}

static float max(float a, float b) {
    return (a > b) ? a : b;
}

static void hsl_to_rgb(float h, float s, float l, float *r, float *g,
                       float *b) {
    float a = s * min(l, 1.0 - l);
    float kr = fmod(h * 12, 12);
    float kg = fmod(8 + h * 12, 12);
    float kb = fmod(4 + h * 12, 12);
    *r = l - a * max(-1.0, min(min(kr - 3.0, 9.0 - kr), 1.0));
    *g = l - a * max(-1.0, min(min(kg - 3.0, 9.0 - kg), 1.0));
    *b = l - a * max(-1.0, min(min(kb - 3.0, 9.0 - kb), 1.0));
}

int main(int argc, const char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s [file.obj]\n", argv[0]);
        return 1;
    }

    const char* obj_filename = argv[1];
    FILE* obj_file = fopen(obj_filename, "r");
    if (obj_file == NULL) {
        perror("Unable to open obj file");
        return 1;
    }

    struct obj_obj* obj = obj_read(obj_file);
    if (obj == NULL) {
        fclose(obj_file);
        fprintf(stderr, "Failed to parse obj file");
        return 1;
    }
    fclose(obj_file);

    if (VERBOSE)
        printf("Loaded '%s' with %d vertices\n", obj_filename, obj->v->n);

    SDL_Init(SDL_INIT_VIDEO);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    if (DEBUG)
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);

    if (SDL_GL_SetSwapInterval(-1) < 0)
        SDL_GL_SetSwapInterval(1);

    const int W = 640;
    const int H = 480;

    SDL_Window* window = SDL_CreateWindow(
        "Hello World",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        W, H,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN
    );

    if (window == NULL) {
        fprintf(stderr, "Could not create window: %s\n", SDL_GetError());
        return 1;
    }

    SDL_GLContext context = SDL_GL_CreateContext(window);

    if (VERBOSE) {
        printf("GL_VERSION: %s\nGL_EXTENSIONS: %s\nGL_VENDOR: %s\nGL_RENDERER: %s\n",
               glGetString(GL_VERSION), glGetString(GL_EXTENSIONS),
               glGetString(GL_VENDOR), glGetString(GL_RENDERER));
    }

    int running = 1;

    float h1 = 0.5;
    float s1 = 0.7;
    float l1 = 0.6;

    float h2 = 0.95;
    float s2 = 0.4;
    float l2 = 0.2;

    float t = 0;
    float dt = 0.002;

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

        t += dt;
        if (t > 1 || t < 0)
            dt = -dt;

        float h = h1 + (h2 - h1) * t;
        float s = s1 + (s2 - s1) * t;
        float l = l1 + (l2 - l1) * t;

        float r, g, b;
        hsl_to_rgb(h, s, l, &r, &g, &b);

        glViewport(0, 0, W, H);
        glClearColor(r, g, b, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        SDL_GL_SwapWindow(window);
    }

    obj_free(obj);

    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
