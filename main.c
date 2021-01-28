#include <stdlib.h>
#include <stdio.h>
#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
#include <SDL.h>

int main(int argc, const char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window* window = SDL_CreateWindow(
        "Hello",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        640,
        480,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN
    );

    if (window == NULL) {
        printf("Could not create window: %s\n", SDL_GetError());
        return 1;
    }

    SDL_GLContext context = SDL_GL_CreateContext(window);

    int running = 1;

    float r2 = (float)rand() / (float)RAND_MAX;
    float g2 = (float)rand() / (float)RAND_MAX;
    float b2 = (float)rand() / (float)RAND_MAX;

    float r1 = 0.4;
    float g1 = 0.6;
    float b1 = 0.5;

    float t = 0;
    float dt = 0.01;

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

        float r = r1 + (r2 - r1) * t;
        float g = g1 + (g2 - g1) * t;
        float b = b1 + (b2 - b1) * t;

        glViewport(0, 0, 640, 480);
        glClearColor(r, g, b, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        SDL_GL_SwapWindow(window);
    }

    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
