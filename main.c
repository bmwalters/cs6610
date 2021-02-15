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

const float PI = 3.1415926535;

static float min(float a, float b) { return (a < b) ? a : b; }

static float max(float a, float b) { return (a > b) ? a : b; }

/* column-major order */

static void matmul(float lm[4][4], float rm[4][4], float outm[4][4]) {
    float *l = lm[0];
    float *r = rm[0];
    float *out = outm[0];
    for (int i = 0; i < 16; i += 4, out += 4) {
        float a[4], b[4], c[4], d[4];
        for (int j = 0; j < 4; ++j)
            a[j] = l[j] * r[i];
        for (int j = 0; j < 4; ++j)
            b[j] = l[4 + j] * r[i + 1];
        for (int j = 0; j < 4; ++j)
            c[j] = l[8 + j] * r[i + 2];
        for (int j = 0; j < 4; ++j)
            d[j] = l[12 + j] * r[i + 3];
        for (int j = 0; j < 4; ++j)
            out[j] = a[j] + b[j] + c[j] + d[j];
    }
}

static void matscale(float out[4][4], float scale) {
    float m[4][4] = {
        {scale, 0, 0, 0}, {0, scale, 0, 0}, {0, 0, scale, 0}, {0, 0, 0, 1}};
    memcpy(out, m, sizeof(float[4][4]));
}

static void mattranslate(float out[4][4], float tx, float ty, float tz) {
    float m[4][4] = {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {tx, ty, tz, 1}};
    memcpy(out, m, sizeof(float[4][4]));
}

static void matrotatex(float out[4][4], float theta) {
    float m[4][4] = {{1, 0, 0, 0},
                     {0, cos(theta), -sin(theta), 0},
                     {0, sin(theta), cos(theta), 0},
                     {0, 0, 0, 1}};
    memcpy(out, m, sizeof(float[4][4]));
}

static void matrotatey(float out[4][4], float theta) {
    float m[4][4] = {{cos(theta), 0, sin(theta), 0},
                     {0, 1, 0, 0},
                     {-sin(theta), 0, cos(theta), 0},
                     {0, 0, 0, 1}};
    memcpy(out, m, sizeof(float[4][4]));
}

static void matrotatez(float out[4][4], float theta) {
    float m[4][4] = {{cos(theta), -sin(theta), 0, 0},
                     {sin(theta), cos(theta), 0, 0},
                     {0, 0, 1, 0},
                     {0, 0, 0, 1}};
    memcpy(out, m, sizeof(float[4][4]));
}

static void matperspective(float out[4][4], float f, float n) {
    float m[4][4] = {
        {n, 0, 0, 0}, {0, n, 0, 0}, {0, 0, n + f, 1}, {0, 0, -f * n, 0}};
    memcpy(out, m, sizeof(float[4][4]));
}

static void matortho(float out[4][4], float l, float r, float b, float t,
                     float f, float n) {
    float m[4][4] = {
        {2 / (r - l), 0, 0, 0},
        {0, 2 / (t - b), 0, 0},
        {0, 0, 2 / (f - n), 0},
        {-(r + l) / (r - l), -(t + b) / (t - b), -(f + n) / (f - n), 1}};
    memcpy(out, m, sizeof(float[4][4]));
}

static void matprint(float m[4][4]) {
    printf("%f %f %f %f\n%f %f %f %f\n%f %f %f %f\n%f %f %f %f\n", m[0][0],
           m[1][0], m[2][0], m[3][0], m[0][1], m[1][1], m[2][1], m[3][1],
           m[0][2], m[1][2], m[2][2], m[3][2], m[0][3], m[1][3], m[2][3],
           m[3][3]);
}

static void bounding_box(const struct obj_obj *obj, struct obj_vertex *outmin,
                         struct obj_vertex *outmax) {
    if (obj->v.n < 1)
        return;
    struct obj_vertex min = obj->v.v[0];
    struct obj_vertex max = obj->v.v[0];
    for (int i = 1; i < obj->v.n; i++) {
        struct obj_vertex vertex = obj->v.v[i];
        if (vertex.x < min.x)
            min.x = vertex.x;
        if (vertex.y < min.y)
            min.y = vertex.y;
        if (vertex.z < min.z)
            min.z = vertex.z;
        if (vertex.x > max.x)
            max.x = vertex.x;
        if (vertex.y > max.y)
            max.y = vertex.y;
        if (vertex.z > max.z)
            max.z = vertex.z;
    }
    *outmin = min;
    *outmax = max;
}

static bool compile_shader_file(const char *const filename, GLuint shader);
static bool compile_shader_program(GLuint program);

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

    struct obj_obj obj;
    obj_init(&obj);
    if (!obj_read(&obj, obj_file)) {
        fprintf(stderr, "Failed to parse obj file");
        return 1;
    }
    fclose(obj_file);

    if (VERBOSE)
        printf("Loaded '%s' with %d vertices\n", obj_filename, obj.v.n);

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

    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    // glBufferData(GL_ARRAY_BUFFER, obj.v.n * sizeof(struct obj_vertex),
    // obj.v.v, GL_STATIC_DRAW);

#define nlines 3
    float lines[nlines][3][3] = {
        {{0, 0, 0}, {1, 0, 0}}, {{0, 0, 0}, {0, 1, 0}}, {{0, 0, 0}, {0, 0, 1}}};
    glBufferData(GL_ARRAY_BUFFER, nlines * 6 * sizeof(float), lines,
                 GL_STATIC_DRAW);

    GLuint pos = glGetAttribLocation(program, "pos");
    glEnableVertexAttribArray(pos);
    glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glUseProgram(program);

    struct obj_vertex obj_min, obj_max;
    bounding_box(&obj, &obj_min, &obj_max);

    float pl = -500;
    float pr = 500;
    float pb = -500;
    float pt = 500;
    float pf = 500; // TODO: Values for these
    float pn = 0.1;

    float rx = 0;
    float ry = 0;
    float rz = 0;

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
                case SDLK_F6:
                    compile_shader_program(program);
                    break;
                default:
                    break;
                }
                break;
            case SDL_MOUSEMOTION: {
                if (event.motion.state & SDL_BUTTON_LMASK) {
                    ry += event.motion.xrel * 0.005;
                    rx += event.motion.yrel * 0.005;
                } else if (event.motion.state & SDL_BUTTON_RMASK) {
                    pn += event.motion.yrel * 0.005;
                }
                break;
            }
            case SDL_QUIT:
                running = 0;
                break;
            default:
                break;
            }
        }

        // view -> canonical view volume projection
        // "projection matrix"
        float ortho[4][4];
        //        matortho(ortho, pl, pr, pb, pt, pf, pn);
        matscale(ortho, 1);
        float perspective[4][4];
        matscale(perspective, 1);
        /* matperspective(perspective, pf, pn); */
        float projection[4][4];
        matmul(ortho, perspective, projection);

        // world -> view transformation
        // "view matrix"
        float view[4][4];
        mattranslate(view, 0, 0, 0); // TODO (glm::lookAt)

        // model -> world transformation
        // "model matrix"
        float model_to_world_center[4][4];
        mattranslate(model_to_world_center, 0, 0, 0);
        float model[4][4];
        float scale[4][4];
        matscale(scale, 1);
        matmul(scale, model_to_world_center, model);

        // transformations in model space
        float rotz[4][4], roty[4][4], rotx[4][4];
        matrotatez(rotz, rz);
        matrotatey(roty, ry);
        matrotatex(rotx, rx);
        float temp1[4][4], mtrans[4][4];
        matmul(rotz, roty, temp1);
        matmul(temp1, rotx, mtrans);

        // v' = P V M Mt v
        printf("mtrans\n");
        matprint(mtrans);

        printf("model\n");
        matprint(model);

        printf("view\n");
        matprint(view);

        printf("projection\n");
        matprint(projection);

        float temp2[4][4], mvp[4][4];
        matmul(projection, view, temp1);
        matmul(temp1, model, temp2);
        matmul(temp2, mtrans, mvp);

        printf("mvp\n");
        matprint(mvp);

        GLfloat *mvpp = &mvp[0][0];
        glUniformMatrix4fv(glGetUniformLocation(program, "mvp"), 1, GL_FALSE,
                           mvpp);

        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //        glDrawArrays(GL_POINTS, 0, obj.v.n);
        glDrawArrays(GL_LINES, 0, 2 * nlines);

        SDL_GL_SwapWindow(window);
    }

    obj_release(&obj);

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
