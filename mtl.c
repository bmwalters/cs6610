#include "mtl.h"
#include "SDL_pixels.h"
#include "SDL_surface.h"
#include <SDL_image.h>
#include <assert.h>
#include <libgen.h>
#include <stdlib.h>
#include <string.h>

/* mtl_texture_image */

static bool mtl_texture_image_load(struct mtl_texture_image *dest,
                                   const char *mtl_filename,
                                   const char *img_filename);

static void mtl_texture_image_release(struct mtl_texture_image *image);

/* mtl_library vector functions */

void mtl_library_init(struct mtl_library *library) {
    library->n = 0;
    library->c = 0;
    library->v = NULL;
}

static void mtl_release(struct mtl_mtl *mtl) {
    free((char *)mtl->name);
    if (mtl->map_Ka.v != NULL)
        mtl_texture_image_release(&mtl->map_Ka);
    if (mtl->map_Kd.v != NULL)
        mtl_texture_image_release(&mtl->map_Kd);
    if (mtl->map_Ks.v != NULL)
        mtl_texture_image_release(&mtl->map_Ks);
}

void mtl_library_release(struct mtl_library *library) {
    for (size_t i = 0; i < library->n; i++)
        mtl_release(&library->v[i]);
    library->n = 0;
    library->c = 0;
    if (library->v != NULL)
        free(library->v);
    library->v = NULL;
}

static bool mtl_library_add(struct mtl_library *library, struct mtl_mtl *item) {
    while (library->n >= library->c) {
        unsigned int new_c = (library->c == 0) ? 1 : library->c * 2;
        struct mtl_mtl *new_v = calloc(new_c, sizeof(struct mtl_mtl));
        if (new_v == NULL)
            return false;

        library->v =
            memcpy(new_v, library->v, sizeof(struct mtl_mtl) * library->n);
        library->c = new_c;
    }

    library->v[library->n] = *item;
    library->n++;

    return true;
}

/* mtl parsing */

static bool mtl_library_read_from_file(struct mtl_library *library,
                                       const char *filename, FILE *file) {
    enum { nline = 1024 };
    char curline[nline];

    bool has_cur_mtl = false;
    struct mtl_mtl cur_mtl;

    while (fgets(curline, nline, file) != NULL) {
        char *line = curline;
        while (*line == ' ' || *line == '\t')
            line++;
        if (strncmp(line, "newmtl ", 7) == 0) {
            if (has_cur_mtl) {
                if (!mtl_library_add(library, &cur_mtl))
                    return false;
                has_cur_mtl = false;
            }

            struct mtl_mtl next_mtl = {0};
            cur_mtl = next_mtl;
            has_cur_mtl = true;

            char name[nline];
            sscanf(line, "newmtl %s", name);
            // TODO: check scanf return value

            size_t name_len = strlen(name) + 1;
            char *owned_name = calloc(name_len, 1);
            if (owned_name == NULL)
                return false;

            strncpy(owned_name, name, name_len);
            cur_mtl.name = owned_name;
        } else if (strncmp(line, "Ka ", 3) == 0) {
            assert(has_cur_mtl);
            // TODO: support only providing r or r g
            sscanf(line, "Ka %f %f %f", &cur_mtl.Ka[0], &cur_mtl.Ka[1],
                   &cur_mtl.Ka[2]);
            // TODO: check scanf return value
        } else if (strncmp(line, "Kd ", 3) == 0) {
            assert(has_cur_mtl);
            // TODO: support only providing r or r g
            sscanf(line, "Kd %f %f %f", &cur_mtl.Kd[0], &cur_mtl.Kd[1],
                   &cur_mtl.Kd[2]);
            // TODO: check scanf return value
        } else if (strncmp(line, "Ks ", 3) == 0) {
            assert(has_cur_mtl);
            // TODO: support only providing r or r g
            sscanf(line, "Ks %f %f %f", &cur_mtl.Ks[0], &cur_mtl.Ks[1],
                   &cur_mtl.Ks[2]);
            // TODO: check scanf return value
        } else if (strncmp(line, "illum ", 6) == 0) {
            assert(has_cur_mtl);
            sscanf(line, "illum %d", (int *)&cur_mtl.illum);
            // TODO: check scanf return value
        } else if (strncmp(line, "Ns ", 3) == 0) {
            assert(has_cur_mtl);
            sscanf(line, "Ns %f", &cur_mtl.Ns);
            // TODO: check scanf return value
        } else if (strncmp(line, "map_Ka ", 7) == 0) {
            assert(has_cur_mtl);
            char img_filename[nline];
            sscanf(line, "map_Ka %s", img_filename);
            // TODO: check scanf return value
            if (!mtl_texture_image_load(&cur_mtl.map_Ka, filename,
                                        img_filename))
                return false;
        } else if (strncmp(line, "map_Kd ", 7) == 0) {
            assert(has_cur_mtl);
            char img_filename[nline];
            sscanf(line, "map_Kd %s", img_filename);
            // TODO: check scanf return value
            if (!mtl_texture_image_load(&cur_mtl.map_Kd, filename,
                                        img_filename))
                return false;
        } else if (strncmp(line, "map_Ks ", 7) == 0) {
            assert(has_cur_mtl);
            char img_filename[nline];
            sscanf(line, "map_Ks %s", img_filename);
            // TODO: check scanf return value
            if (!mtl_texture_image_load(&cur_mtl.map_Ks, filename,
                                        img_filename))
                return false;
        }
    }

    if (has_cur_mtl)
        if (!mtl_library_add(library, &cur_mtl))
            return false;

    return true;
}

bool mtl_library_read(struct mtl_library *library, const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL)
        return false;

    bool result = mtl_library_read_from_file(library, filename, file);
    fclose(file);
    return result;
}

/* mtl_texture_image loading */

static void vflip_sdl_surface(SDL_Surface *surface) {
    void *temp = malloc(surface->pitch);
    if (temp == NULL)
        return;

    for (int i = 0; i < surface->h; i++) {
        void *row1 = (char *)surface->pixels + i * surface->pitch;
        void *row2 =
            (char *)surface->pixels + (surface->h - i - 1) * surface->pitch;
        memcpy(temp, row1, surface->pitch);
        memcpy(row1, row2, surface->pitch);
        memcpy(row2, temp, surface->pitch);
    }

    free(temp);
}

static bool mtl_texture_image_load(struct mtl_texture_image *dest,
                                   const char *mtl_filename,
                                   const char *img_filename) {
    size_t filename_len = strlen(mtl_filename) + strlen(img_filename) + 1;
    char *filename = calloc(filename_len, 1);

    /* read the directory of the mtl file into filename */
    strncpy(filename, mtl_filename, filename_len);
    char *mtl_directory = dirname(filename);
    strncpy(filename, mtl_directory, filename_len);
    size_t mtl_directory_len = strlen(mtl_directory);

    /* concatenate /<img_filename_relative> to the directory */
    assert(mtl_directory_len < (filename_len - 1));
    strcat(filename, "/");
    strncat(filename, img_filename, filename_len - mtl_directory_len - 1);

    SDL_Surface *surface;
    if ((surface = IMG_Load(filename)) == NULL)
        return false;

    SDL_PixelFormatEnum desired_format = SDL_PIXELFORMAT_ABGR8888;
    if (surface->format->format != desired_format) {
        SDL_PixelFormat *dest_format = SDL_AllocFormat(desired_format);
        SDL_Surface *converted = SDL_ConvertSurface(surface, dest_format, 0);
        SDL_FreeFormat(dest_format);
        SDL_FreeSurface(surface);
        if (converted == NULL)
            return false;
        surface = converted;
    }

    /* OpenGL places 0,0 at the bottom left; convention for images is top left
     */
    vflip_sdl_surface(surface);

    dest->w = surface->w;
    dest->h = surface->h;

    void *pixel_data = calloc(surface->h, surface->pitch);
    if (pixel_data == NULL) {
        SDL_FreeSurface(surface);
        return false;
    }

    memcpy(pixel_data, surface->pixels, surface->h * surface->pitch);
    SDL_FreeSurface(surface);

    dest->v = pixel_data;

    return true;
}

static void mtl_texture_image_release(struct mtl_texture_image *image) {
    image->w = 0;
    image->h = 0;
    free(image->v);
    image->v = NULL;
}
