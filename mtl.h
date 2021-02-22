#pragma once

#include <stdbool.h>
#include <stdio.h>

enum mtl_illumination_model {
    MTL_ILLUMINATION_MODEL_COLOR = 0,
    MTL_ILLUMINATION_MODEL_COLOR_AMBIENT,
    MTL_ILLUMINATION_MODEL_HIGHLIGHT,
    MTL_ILLUMINATION_MODEL_REFLECTION_RAY_TRACE,
    MTL_ILLUMINATION_MODEL_GLASS_RAY_TRACE,
    MTL_ILLUMINATION_MODEL_FRESNEL_RAY_TRACE,
    MTL_ILLUMINATION_MODEL_REFRACTION_RAY_TRACE,
    MTL_ILLUMINATION_MODEL_REFRACTION_FRESNEL_RAY_TRACE,
    MTL_ILLUMINATION_MODEL_REFLECTION,
    MTL_ILLUMINATION_MODEL_GLASS,
    MTL_ILLUMINATION_MODEL_SHADOWS
};

/** 8 bytes per channel RGBA, row-major */
struct mtl_texture_image {
    size_t w;
    size_t h;
    unsigned char *v;
};

struct mtl_mtl {
    /** material name */
    const char *name;
    /** ambient color */
    float Ka[3];
    /** diffuse color */
    float Kd[3];
    /** specular color */
    float Ks[3];
    /** illumination model */
    enum mtl_illumination_model illum;
    /** specular exponent */
    float Ns;
    /** ambient texture map */
    struct mtl_texture_image map_Ka;
    /** diffuse texture map*/
    struct mtl_texture_image map_Kd;
    /** specular texture map*/
    struct mtl_texture_image map_Ks;
};

struct mtl_library {
    size_t n;
    size_t c;
    struct mtl_mtl *v;
};

/** must be called before using an `mtl_library` */
void mtl_library_init(struct mtl_library *library);

/** must be called before deallocating an `mtl_library` */
void mtl_library_release(struct mtl_library *library);

/** appends material definitions from file `filename` into `library` until EOF */
bool mtl_library_read(struct mtl_library *library, const char *filename);
