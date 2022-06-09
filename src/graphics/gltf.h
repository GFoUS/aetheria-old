#pragma once

#include "core/core.h"

#include "cJSON.h"

typedef struct gltf_accessor_t gltf_accessor;
typedef struct gltf_buffer_t gltf_buffer;
typedef struct gltf_buffer_view_t gltf_buffer_view;
typedef struct gltf_image_t gltf_image;
typedef struct gltf_material_t gltf_material;
typedef struct gltf_mesh_t gltf_mesh;
typedef struct gltf_node_t gltf_node;
typedef struct gltf_sampler_t gltf_sampler;
typedef struct gltf_scene_t gltf_scene;
typedef struct gltf_texture_t gltf_texture;
typedef struct gltf_gltf_t gltf_gltf;

typedef enum {
    ACCESSOR_COMPONENT_TYPE_I8    = 5120,
    ACCESSOR_COMPONENT_TYPE_U8    = 5121,
    ACCESSOR_COMPONENT_TYPE_I16   = 5122,
    ACCESSOR_COMPONENT_TYPE_U16   = 5123,
    ACCESSOR_COMPONENT_TYPE_U32   = 5125,
    ACCESSOR_COMPONENT_TYPE_FLOAT = 5126
} gltf_accessor_component_type;

typedef enum {
    ACCESSOR_ELEMENT_TYPE_SCALAR = 1,
    ACCESSOR_ELEMENT_TYPE_VEC2   = 2,
    ACCESSOR_ELEMENT_TYPE_VEC3   = 3,
    ACCESSOR_ELEMENT_TYPE_VEC4   = 4,
    ACCESSOR_ELEMENT_TYPE_MAT2   = 4,
    ACCESSOR_ELEMENT_TYPE_MAT3   = 9,
    ACCESSOR_ELEMENT_TYPE_MAT4   = 16
} gltf_accessor_element_type;

typedef struct gltf_accessor_t {
    gltf_gltf* gltf;
    u32 id;

    gltf_buffer_view* bufferView;
    size_t byteOffset;
    gltf_accessor_component_type componentType;
    bool normalized;
    u64 count;
    gltf_accessor_element_type type;
    float max[16];
    float min[16];
} gltf_accessor;

typedef struct gltf_buffer_t {
    gltf_gltf* gltf;
    u32 id;

    const char* uri;
    size_t byteLength;

    void* data;
} gltf_buffer;

typedef enum {
    BUFFER_VIEW_TARGET_UNDEFINED     = 0,
    BUFFER_VIEW_TARGET_VERTEX_BUFFER = 34962,
    BUFFER_VIEW_TARGET_INDEX_BUFFER  = 34963 
} gltf_buffer_view_target;

typedef struct gltf_buffer_view_t {
    gltf_gltf* gltf;
    u32 id;

    gltf_buffer* buffer;
    size_t byteOffset;
    size_t byteLength;
    size_t byteStride;
    gltf_buffer_view_target target;
} gltf_buffer_view;

typedef struct gltf_image_t {
    gltf_gltf* gltf;
    u32 id;

    const char* uri;
} gltf_image;

typedef struct {
    gltf_texture* texture;
    u32 texCoord;
    bool useDefault;
    float scale;
} gltf_material_normal_texture;

typedef struct {
    gltf_texture* texture;
    u32 texCoord;
    bool useDefault;
    float strength;
} gltf_material_occlusion_texture;

typedef struct {
    gltf_texture* texture;
    u32 texCoord;
    bool useDefault;
} gltf_texture_reference;

typedef struct {
    float baseColorFactor[4];
    gltf_texture_reference baseColorTexture;
    float metallicFactor;
    float roughnessFactor;
    gltf_texture_reference metallicRoughnessTexture;
} gltf_material_pbr;

typedef enum {
    ALPHA_MODE_OPAQUE,
    ALPHA_MODE_MASK,
    ALPHA_MODE_BLEND
} gltf_material_alpha_mode;

typedef struct gltf_material_t {
    gltf_gltf* gltf;
    u32 id;

    gltf_material_pbr pbr;
    gltf_material_normal_texture normalTexture;
    gltf_material_occlusion_texture occlusionTexture;
    gltf_texture_reference emissiveTexture;
    float emissiveFactor[3];
    gltf_material_alpha_mode alphaMode;
    float alphaCutoff;
    bool doubleSided;
} gltf_material;

typedef enum {
    PRIMITIVE_MODE_POINTS,
    PRIMITIVE_MODE_LINES,
    PRIMITIVE_MODE_LINE_LOOP,
    PRIMITIVE_MODE_LINE_STRIP,
    PRIMITIVE_MODE_TRIANGLES,
    PRIMITIVE_MODE_TRIANGLE_STRIP,
    PRIMITIVE_MODE_TRIANGLE_FAN
} gltf_mesh_primitive_mode;

typedef struct {
    gltf_accessor* position;
    gltf_accessor* normal;
    gltf_accessor* baseColorTextureUV;

    gltf_accessor* index;
    gltf_material* material;
    gltf_mesh_primitive_mode mode;
    // TODO: targets
} gltf_mesh_primitive;

typedef struct gltf_mesh_t {
    gltf_gltf* gltf;
    u32 id;

    u32 numPrimitives;
    gltf_mesh_primitive* primitives;

    u32 numWeights;
    float* weights;
} gltf_mesh;

typedef struct gltf_node_t {
    gltf_gltf* gltf;
    u32 id;

    u32 numChildren;
    gltf_node** children;

    float matrix[16];
    gltf_mesh* mesh;

    u32 numWeights;
    float* weights;
    // TODO: Cameras and Skins
} gltf_node;

typedef enum {
    SAMPLER_FILTER_NEAREST = 9728,
    SAMPLER_FILTER_LINEAR  = 9729,
    SAMPLER_FILTER_NEAREST_MIPMAP_NEAREST = 9984,
    SAMPLER_FILTER_LINEAR_MIPMAP_NEAREST  = 9985,
    SAMPLER_FILTER_NEAREST_MIPMAP_LINEAR  = 9986,
    SAMPLER_FILTER_LINEAR_MIPMAP_LINEAR   = 9987
} gltf_sampler_filter;

typedef enum {
    SAMPLER_WRAP_MODE_CLAMP_TO_EDGE   = 33071,
    SAMPLER_WRAP_MODE_MIRRORED_REPEAT = 33648,
    SAMPLER_WRAP_MODE_REPEAT          = 10497
} gltf_sampler_wrap_mode;

typedef struct gltf_sampler_t {
    gltf_gltf* gltf;
    u32 id;

    gltf_sampler_filter magFilter;
    gltf_sampler_filter minFilter;
    gltf_sampler_wrap_mode wrapS;
    gltf_sampler_wrap_mode wrapT;
} gltf_sampler;

typedef struct gltf_scene_t {
    gltf_gltf* gltf;
    u32 id;

    u32 numNodes;
    gltf_node** nodes;
} gltf_scene;

typedef struct gltf_texture_t {
    gltf_gltf* gltf;
    u32 id;

    gltf_sampler* sampler;
    gltf_image* source;
} gltf_texture;

typedef struct gltf_gltf_t {
    gltf_scene* scene;
    const char* path;
    cJSON* json;

    u32 numAccessors;
    gltf_accessor* accessors;

    u32 numBuffers;
    gltf_buffer* buffers;

    u32 numBufferViews;
    gltf_buffer_view* bufferViews;

    u32 numImages;
    gltf_image* images;

    u32 numMaterials;
    gltf_material* materials;

    u32 numMeshes;
    gltf_mesh* meshes;

    u32 numNodes;
    gltf_node* nodes;

    u32 numSamplers;
    gltf_sampler* samplers;

    u32 numScenes;
    gltf_scene* scenes;

    u32 numTextures;
    gltf_texture* textures;
} gltf_gltf;

char* gltf_merge_paths(const char* path, const char* uri);

gltf_gltf* gltf_load_file(const char* path);
void gltf_unload(gltf_gltf* gltf);

size_t gltf_get_accessor_offset(gltf_accessor* accessor);
