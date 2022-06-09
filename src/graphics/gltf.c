#include "gltf.h"

#include "cglm/cglm.h"

// UTILITY FUNCTIONS
int cJSON_GetIntOptional(const cJSON* data, const char* key, int defaultValue) {
    const cJSON* value = cJSON_GetObjectItemCaseSensitive(data, key);
    if (value == NULL) {
        return defaultValue;
    } else {
        return value->valueint;
    }
}

double cJSON_GetDoubleOptional(const cJSON* data, const char* key, double defaultValue) {
    const cJSON* value = cJSON_GetObjectItemCaseSensitive(data, key);
    if (value == NULL) {
        return defaultValue;
    } else {
        return value->valuedouble;
    }
}

void gltf_get_texcoord_string(u32 texCoord, char result[14]) {
    char texCoordBase[9] = "TEXCOORD_";
    memcpy(result, texCoordBase, 9);
    sprintf_s(&result[9], 5, "%d", texCoord);
}

char* gltf_merge_paths(const char* path, const char* uri) {
    size_t uriLength = strlen(uri);
    char* lastSlash = strrchr(path, (int)"/"[0]);
    u64 lastSlashPos = lastSlash - path;

    char* result = malloc(lastSlashPos + 1 + uriLength + 1); // lastSlashPos + 1 is the path up to and including the last slash, uriLength will be needed for the new path, and the final + 1 is for the null-termination character 
    CLEAR_MEMORY_ARRAY(result, lastSlashPos + 1 + uriLength + 1);
    memcpy(result, path, lastSlashPos + 1);
    memcpy(&result[lastSlashPos + 1], uri, uriLength);

    return result;
}

// LOADING FUNCTIONS
void gltf_load_accessor(gltf_gltf* gltf, const cJSON* data, gltf_accessor* accessor) {
    const cJSON* bufferView = cJSON_GetObjectItemCaseSensitive(data, "bufferView");
    if (bufferView == NULL) {
        FATAL("Accessors wihtout bufferViews are not supported");
    }
    accessor->bufferView = &gltf->bufferViews[bufferView->valueint];
    
    const cJSON* byteOffset = cJSON_GetObjectItemCaseSensitive(data, "byteOffset");
    if (byteOffset == NULL) {
        accessor->byteOffset = 0;
    } else {
        accessor->byteOffset = byteOffset->valueint;
    }

    accessor->componentType = (gltf_accessor_component_type)cJSON_GetObjectItemCaseSensitive(data, "componentType")->valueint;
    accessor->normalized = cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(data, "normalized"));
    accessor->count = cJSON_GetObjectItemCaseSensitive(data, "count")->valueint;
    
    const char* type = cJSON_GetObjectItemCaseSensitive(data, "type")->valuestring;
    if (strcmp(type, "SCALAR") == 0)    accessor->type = ACCESSOR_ELEMENT_TYPE_SCALAR; 
    else if (strcmp(type, "VEC2") == 0) accessor->type = ACCESSOR_ELEMENT_TYPE_VEC2; 
    else if (strcmp(type, "VEC3") == 0) accessor->type = ACCESSOR_ELEMENT_TYPE_VEC3; 
    else if (strcmp(type, "VEC4") == 0) accessor->type = ACCESSOR_ELEMENT_TYPE_VEC4; 
    else if (strcmp(type, "MAT2") == 0) accessor->type = ACCESSOR_ELEMENT_TYPE_MAT2; 
    else if (strcmp(type, "MAT3") == 0) accessor->type = ACCESSOR_ELEMENT_TYPE_MAT3; 
    else if (strcmp(type, "MAT4") == 0) accessor->type = ACCESSOR_ELEMENT_TYPE_MAT4; 

    const cJSON* maxJSON = cJSON_GetObjectItemCaseSensitive(data, "max");
    const cJSON* minJSON = cJSON_GetObjectItemCaseSensitive(data, "max");
    u32 numMaxMinValues = cJSON_GetArraySize(maxJSON);
    for (u32 i = 0; i < numMaxMinValues; i++) {
        accessor->max[i] = (float)cJSON_GetArrayItem(maxJSON, i)->valuedouble;
        accessor->min[i] = (float)cJSON_GetArrayItem(minJSON, i)->valuedouble;
    }
}

void gltf_load_buffer(gltf_gltf* gltf, const cJSON* data, gltf_buffer* buffer) {
    const cJSON* uri = cJSON_GetObjectItemCaseSensitive(data, "uri");
    if (uri == NULL) {
        FATAL("Buffers without URIs are not supported yet");
    }
    buffer->uri = uri->valuestring;
    buffer->byteLength = (size_t)cJSON_GetObjectItemCaseSensitive(data, "byteLength")->valueint;

    char* bufferFilePath = gltf_merge_paths(gltf->path, buffer->uri);
    FILE* bufferFile;
    fopen_s(&bufferFile, bufferFilePath, "rb");
    if (!bufferFile) {
        FATAL("Unable to open buffer file: %s", bufferFilePath);
    }
    free(bufferFilePath);

    buffer->data = malloc(buffer->byteLength);
    fread(buffer->data, buffer->byteLength, 1, bufferFile);
    fclose(bufferFile);
}

void gltf_load_buffer_view(gltf_gltf* gltf, const cJSON* data, gltf_buffer_view* bufferView) {
    bufferView->buffer = &gltf->buffers[cJSON_GetObjectItemCaseSensitive(data, "buffer")->valueint];
    
    const cJSON* byteOffset = cJSON_GetObjectItemCaseSensitive(data, "byteOffset");
    if (byteOffset == NULL) {
        bufferView->byteOffset = 0;
    } else {
        bufferView->byteOffset = byteOffset->valueint;
    }

    bufferView->byteLength = cJSON_GetObjectItemCaseSensitive(data, "byteLength")->valueint;
    
    const cJSON* byteStride = cJSON_GetObjectItemCaseSensitive(data, "byteStride");
    if (byteStride == NULL) {
        bufferView->byteStride = 0;
    } else {
        bufferView->byteStride = byteStride->valueint;
    }

    const cJSON* target = cJSON_GetObjectItemCaseSensitive(data, "target");
    if (target == NULL) {
        bufferView->target = BUFFER_VIEW_TARGET_UNDEFINED;
    } else {
        bufferView->target = (gltf_buffer_view_target)target->valueint;
    }
}

void gltf_load_texture_reference(gltf_gltf* gltf, const cJSON* data, gltf_texture_reference* ref) {
    if (data == NULL) {
        ref->useDefault = true;
        return;
    } else {
        ref->texture = &gltf->textures[cJSON_GetObjectItemCaseSensitive(data, "index")->valueint];
        const cJSON* texCoord = cJSON_GetObjectItemCaseSensitive(data, "texCoord");
        ref->texCoord = cJSON_GetIntOptional(data, "texCoord", 0);
    }
}

void gltf_load_material(gltf_gltf* gltf, const cJSON* data, gltf_material* material) {
    const cJSON* pbrMetallicRoughness = cJSON_GetObjectItemCaseSensitive(data, "pbrMetallicRoughness");
    const cJSON* baseColorFactor = cJSON_GetObjectItemCaseSensitive(pbrMetallicRoughness, "baseColorFactor");
    if (baseColorFactor == NULL) {
        float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
        memcpy(material->pbr.baseColorFactor, color, sizeof(float) * 4);
    } else {
        for (u32 i = 0; i < 4; i++) {
            material->pbr.baseColorFactor[i] = (float)cJSON_GetArrayItem(baseColorFactor, i)->valuedouble;
        }
    }
    
    gltf_load_texture_reference(gltf, cJSON_GetObjectItemCaseSensitive(pbrMetallicRoughness, "baseColorTexture"), &material->pbr.baseColorTexture);
    material->pbr.metallicFactor = (float)cJSON_GetDoubleOptional(pbrMetallicRoughness, "metallicFactor", 1);
    material->pbr.roughnessFactor = (float)cJSON_GetDoubleOptional(pbrMetallicRoughness, "roughnessFactor", 1);
    gltf_load_texture_reference(gltf, cJSON_GetObjectItemCaseSensitive(pbrMetallicRoughness, "metallicRoughnessTexture"), &material->pbr.metallicRoughnessTexture);

    const cJSON* normalTexture = cJSON_GetObjectItemCaseSensitive(data, "normalTexture");
    gltf_load_texture_reference(gltf, normalTexture, (gltf_texture_reference*)&material->normalTexture); // first 3 members are the same so this works fine
    material->normalTexture.scale = (float)cJSON_GetDoubleOptional(normalTexture, "scale", 1);

    const cJSON* occlusionTexture = cJSON_GetObjectItemCaseSensitive(data, "occlusionTexture");
    gltf_load_texture_reference(gltf, occlusionTexture, (gltf_texture_reference*)&material->occlusionTexture);
    material->occlusionTexture.strength = (float)cJSON_GetDoubleOptional(occlusionTexture, "strength", 1);

    gltf_load_texture_reference(gltf, cJSON_GetObjectItemCaseSensitive(data, "emissiveTexture"), &material->emissiveTexture);

    const cJSON* emissiveFactor = cJSON_GetObjectItemCaseSensitive(data, "emissiveFactor");
    if (emissiveFactor == NULL) {
        float emmissive[3] = {0.0f, 0.0f, 0.0f};
        memcpy(material->emissiveFactor, emmissive, sizeof(float) * 3);
    } else {
        for (u32 i = 0; i < 3; i++) {
            material->emissiveFactor[i] = (float)cJSON_GetArrayItem(emissiveFactor, i)->valuedouble;
        }
    }

    const cJSON* alphaMode = cJSON_GetObjectItemCaseSensitive(data, "alphaMode");
    if (alphaMode == NULL) {
        material->alphaMode = ALPHA_MODE_OPAQUE;
    } else {
        if (strcmp(alphaMode->valuestring, "OPAQUE"))     material->alphaMode = ALPHA_MODE_OPAQUE;
        else if (strcmp(alphaMode->valuestring, "MASK"))  material->alphaMode = ALPHA_MODE_MASK;
        else if (strcmp(alphaMode->valuestring, "BLEND")) material->alphaMode = ALPHA_MODE_BLEND;
    }

    material->alphaCutoff = (float)cJSON_GetDoubleOptional(data, "alphaCutoff", 0.5f);
    material->doubleSided = cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(data, "doubleSided"));
}

void gltf_load_mesh(gltf_gltf* gltf, const cJSON* data, gltf_mesh* mesh) {
    const cJSON* primitives = cJSON_GetObjectItemCaseSensitive(data, "primitives");
    mesh->numPrimitives = cJSON_GetArraySize(primitives);
    mesh->primitives = malloc(sizeof(gltf_mesh_primitive) * mesh->numPrimitives);
    CLEAR_MEMORY_ARRAY(mesh->primitives, mesh->numPrimitives);

    for (u32 i = 0; i < mesh->numPrimitives; i++) {
        gltf_mesh_primitive* primitive = &mesh->primitives[i];
        const cJSON* primitveJSON = cJSON_GetArrayItem(primitives, i);

        const cJSON* material = cJSON_GetObjectItemCaseSensitive(primitveJSON, "material");
        if (material == NULL) {
            FATAL("Models without materials are not supported");
        }
        primitive->material = &gltf->materials[material->valueint];

        const cJSON* attributes = cJSON_GetObjectItemCaseSensitive(primitveJSON, "attributes");
        
        primitive->position = &gltf->accessors[cJSON_GetObjectItemCaseSensitive(attributes, "POSITION")->valueint];
        primitive->normal = &gltf->accessors[cJSON_GetObjectItemCaseSensitive(attributes, "NORMAL")->valueint];

        if (!primitive->material->pbr.baseColorTexture.useDefault) {
            char baseColorTextureTexCoord[14];
            gltf_get_texcoord_string(primitive->material->pbr.baseColorTexture.texCoord, baseColorTextureTexCoord);
            primitive->baseColorTextureUV = &gltf->accessors[cJSON_GetObjectItemCaseSensitive(attributes, baseColorTextureTexCoord)->valueint];
        } else {
            primitive->baseColorTextureUV = NULL;
        }
        
        primitive->index = &gltf->accessors[cJSON_GetObjectItemCaseSensitive(primitveJSON, "indices")->valueint];
        primitive->mode = (gltf_mesh_primitive_mode)cJSON_GetIntOptional(primitveJSON, "mode", 4);
    }

    const cJSON* weights = cJSON_GetObjectItemCaseSensitive(data, "weights");
    mesh->numWeights = cJSON_GetArraySize(weights);
    mesh->weights = malloc(sizeof(float) * mesh->numWeights);
    CLEAR_MEMORY_ARRAY(mesh->weights, mesh->numWeights);

    for (u32 i = 0; i < mesh->numWeights; i++) {
        mesh->weights[i] = (float)cJSON_GetArrayItem(weights, i)->valuedouble;
    }
}

void gltf_load_node(gltf_gltf* gltf, const cJSON* data, gltf_node* node) {
    const cJSON* children = cJSON_GetObjectItemCaseSensitive(data, "children");
    node->numChildren = cJSON_GetArraySize(children);
    node->children = malloc(sizeof(gltf_node*) * node->numChildren);
    CLEAR_MEMORY_ARRAY(node->children, node->numChildren);

    for (u32 i = 0; i < node->numChildren; i++) {
        node->children[i] = &gltf->nodes[cJSON_GetArrayItem(children, i)->valueint];
    }
    
    const cJSON* matrix = cJSON_GetObjectItemCaseSensitive(data, "matrix");
    if (matrix != NULL) {
        for (u32 i = 0; i < 16; i++) {
            node->matrix[i] = (float)cJSON_GetArrayItem(matrix, i)->valuedouble;
        }
    } else {
        glm_mat4_identity((vec4*)&node->matrix); // If the matrix isn't specified, we will need an identity matrix to make the matrix ourselves
    }

    const cJSON* scale = cJSON_GetObjectItemCaseSensitive(data, "scale");
    const cJSON* rotation = cJSON_GetObjectItemCaseSensitive(data, "rotation");
    const cJSON* translation = cJSON_GetObjectItemCaseSensitive(data, "translation");
    if (scale != NULL) {
        vec3 scaleValues = {(float)cJSON_GetArrayItem(scale, 0)->valuedouble, 
                            (float)cJSON_GetArrayItem(scale, 1)->valuedouble, 
                            (float)cJSON_GetArrayItem(scale, 2)->valuedouble};
        glm_scale((vec4*)node->matrix, scaleValues);
    }
    if (rotation != NULL) {
        versor rotationValues = {(float)cJSON_GetArrayItem(rotation, 0)->valuedouble, 
                                (float)cJSON_GetArrayItem(rotation, 1)->valuedouble, 
                                (float)cJSON_GetArrayItem(rotation, 2)->valuedouble, 
                                (float)cJSON_GetArrayItem(rotation, 3)->valuedouble};
        glm_quat_rotate((vec4*)node->matrix, rotationValues, (vec4*)node->matrix);
    }
    if (translation != NULL) {
        vec3 translationValues = {(float)cJSON_GetArrayItem(translation, 0)->valuedouble, 
                                (float)cJSON_GetArrayItem(translation, 1)->valuedouble, 
                                (float)cJSON_GetArrayItem(translation, 2)->valuedouble};
        glm_translate((vec4*)node->matrix, translationValues);
    }

    const cJSON* mesh = cJSON_GetObjectItemCaseSensitive(data, "mesh");
    if (mesh == NULL) {
        node->mesh = NULL;
    } else {
        node->mesh = &gltf->meshes[mesh->valueint];
    }

    const cJSON* weights = cJSON_GetObjectItemCaseSensitive(data, "weights");
    node->numWeights = cJSON_GetArraySize(weights);
    node->weights = malloc(sizeof(float) * node->numWeights);
    CLEAR_MEMORY_ARRAY(node->weights, node->numWeights);

    for (u32 i = 0; i < node->numWeights; i++) {
        node->weights[i] = (float)cJSON_GetArrayItem(weights, i)->valuedouble;
    }
}

void gltf_load_sampler(gltf_gltf* gltf, const cJSON* data, gltf_sampler* sampler) {
    sampler->magFilter = (gltf_sampler_filter)cJSON_GetIntOptional(data, "magFilter", 9728);
    sampler->minFilter = (gltf_sampler_filter)cJSON_GetIntOptional(data, "minFilter", 9728);
    sampler->wrapS = (gltf_sampler_wrap_mode)cJSON_GetIntOptional(data, "wrapS", 10497);
    sampler->wrapT = (gltf_sampler_wrap_mode)cJSON_GetIntOptional(data, "wrapT", 10497);
}

void gltf_load_scene(gltf_gltf* gltf, const cJSON* data, gltf_scene* scene) {
    const cJSON* nodes = cJSON_GetObjectItemCaseSensitive(data, "nodes");
    scene->numNodes = cJSON_GetArraySize(nodes);
    scene->nodes = malloc(sizeof(gltf_node*) * scene->numNodes);
    CLEAR_MEMORY_ARRAY(scene->nodes, scene->numNodes);

    for (u32 i = 0;i < scene->numNodes; i++) {
        scene->nodes[i] = &gltf->nodes[cJSON_GetArrayItem(nodes, i)->valueint];
    }
}

void gltf_load_texture(gltf_gltf* gltf, const cJSON* data, gltf_texture* texture) {
    const cJSON* sampler = cJSON_GetObjectItemCaseSensitive(data, "sampler");
    if (sampler == NULL) {
        FATAL("Texture without a sampler is not yet supported");
    }
    texture->sampler = &gltf->samplers[sampler->valueint];

    const cJSON* source = cJSON_GetObjectItemCaseSensitive(data, "source");
    if (source == NULL) {
        FATAL("Textures without sources are not allowed");    
    }
    texture->source = &gltf->images[source->valueint];
}

gltf_gltf* gltf_load_file(const char* path) {
    gltf_gltf* gltf = malloc(sizeof(gltf_gltf));
    gltf->path = path;

    FILE* gltfFile = NULL;
    fopen_s(&gltfFile, path, "r");
    if (!gltfFile) {
        FATAL("Unable to open file: %s", path);
    }

    fseek(gltfFile, 0L, SEEK_END);
    u64 gltfFileSize = ftell(gltfFile);
    fseek(gltfFile, 0L, SEEK_SET);
    char* gltfText = malloc(gltfFileSize);
    fread(gltfText, gltfFileSize, 1, gltfFile);
    fclose(gltfFile);
    gltf->json = cJSON_Parse(gltfText);
    free(gltfText);

    const cJSON* accessors = cJSON_GetObjectItemCaseSensitive(gltf->json, "accessors");
    const cJSON* buffers = cJSON_GetObjectItemCaseSensitive(gltf->json, "buffers");
    const cJSON* bufferViews = cJSON_GetObjectItemCaseSensitive(gltf->json, "bufferViews");
    const cJSON* images = cJSON_GetObjectItemCaseSensitive(gltf->json, "images");
    const cJSON* materials = cJSON_GetObjectItemCaseSensitive(gltf->json, "materials");
    const cJSON* meshes = cJSON_GetObjectItemCaseSensitive(gltf->json, "meshes");
    const cJSON* nodes = cJSON_GetObjectItemCaseSensitive(gltf->json, "nodes");
    const cJSON* samplers = cJSON_GetObjectItemCaseSensitive(gltf->json, "samplers");
    const cJSON* scenes = cJSON_GetObjectItemCaseSensitive(gltf->json, "scenes");
    const cJSON* textures = cJSON_GetObjectItemCaseSensitive(gltf->json, "textures");

    gltf->numAccessors = cJSON_GetArraySize(accessors);
    gltf->accessors = malloc(sizeof(gltf_accessor) * gltf->numAccessors);
    CLEAR_MEMORY_ARRAY(gltf->accessors, gltf->numAccessors);

    gltf->numBuffers = cJSON_GetArraySize(buffers);
    gltf->buffers = malloc(sizeof(gltf_buffer) * gltf->numBuffers);
    CLEAR_MEMORY_ARRAY(gltf->buffers, gltf->numBuffers);

    gltf->numBufferViews = cJSON_GetArraySize(bufferViews);
    gltf->bufferViews = malloc(sizeof(gltf_buffer_view) * gltf->numBufferViews);
    CLEAR_MEMORY_ARRAY(gltf->bufferViews, gltf->numBufferViews);

    gltf->numImages = cJSON_GetArraySize(images);
    gltf->images = malloc(sizeof(gltf_image) * gltf->numImages);
    CLEAR_MEMORY_ARRAY(gltf->images, gltf->numImages);

    gltf->numMaterials = cJSON_GetArraySize(materials);
    gltf->materials = malloc(sizeof(gltf_material) * gltf->numMaterials);
    CLEAR_MEMORY_ARRAY(gltf->materials, gltf->numMaterials);

    gltf->numMeshes = cJSON_GetArraySize(meshes);
    gltf->meshes = malloc(sizeof(gltf_mesh) * gltf->numMeshes);
    CLEAR_MEMORY_ARRAY(gltf->meshes, gltf->numMeshes);

    gltf->numNodes = cJSON_GetArraySize(nodes);
    gltf->nodes = malloc(sizeof(gltf_node) * gltf->numNodes);
    CLEAR_MEMORY_ARRAY(gltf->nodes, gltf->numNodes);

    gltf->numSamplers = cJSON_GetArraySize(samplers);
    gltf->samplers = malloc(sizeof(gltf_sampler) * gltf->numSamplers);
    CLEAR_MEMORY_ARRAY(gltf->samplers, gltf->numSamplers);

    gltf->numScenes = cJSON_GetArraySize(scenes);
    gltf->scenes = malloc(sizeof(gltf_scene) * gltf->numScenes);
    CLEAR_MEMORY_ARRAY(gltf->scenes, gltf->numScenes);
    gltf->scene = &gltf->scenes[cJSON_GetIntOptional(gltf->json, "scene", 0)];

    gltf->numTextures = cJSON_GetArraySize(textures);
    gltf->textures = malloc(sizeof(gltf_texture) * gltf->numTextures);
    CLEAR_MEMORY_ARRAY(gltf->textures, gltf->numTextures);

    const cJSON* accessor;
    u32 i = 0;
    cJSON_ArrayForEach(accessor, accessors) {
        gltf->accessors[i].id = i;
        gltf_load_accessor(gltf, accessor, &gltf->accessors[i]);
        i++;
    }

    const cJSON* buffer;
    i = 0;
    cJSON_ArrayForEach(buffer, buffers) {
        gltf->buffers[i].id = i;
        gltf_load_buffer(gltf, buffer, &gltf->buffers[i]);
        i++;
    }

    const cJSON* bufferView;
    i = 0;
    cJSON_ArrayForEach(bufferView, bufferViews) {
        gltf->bufferViews[i].id = i;
        gltf_load_buffer_view(gltf, bufferView, &gltf->bufferViews[i]);
        i++;
    }

    const cJSON* image;
    i = 0;
    cJSON_ArrayForEach(image, images) {
        gltf->images[i].id = i;
        const cJSON* uri = cJSON_GetObjectItemCaseSensitive(image, "uri");
        if (uri == NULL) {
            FATAL("Images without URIs are not supported yet");
        }
        gltf->images[i].uri = uri->valuestring;
        i++;
    }

    const cJSON* material;
    i = 0;
    cJSON_ArrayForEach(material, materials) {
        gltf->materials[i].id = i;
        gltf_load_material(gltf, material, &gltf->materials[i]);
        i++;
    }

    const cJSON* mesh;
    i = 0;
    cJSON_ArrayForEach(mesh, meshes) {
        gltf->meshes[i].id = i;
        gltf_load_mesh(gltf, mesh, &gltf->meshes[i]);
        i++;
    }

    const cJSON* node;
    i = 0;
    cJSON_ArrayForEach(node, nodes) {
        gltf->nodes[i].id = i;
        gltf_load_node(gltf, node, &gltf->nodes[i]);
        i++;
    }

    const cJSON* sampler;
    i = 0;
    cJSON_ArrayForEach(sampler, samplers) {
        gltf->samplers[i].id = i;
        gltf_load_sampler(gltf, sampler, &gltf->samplers[i]);
        i++;
    }

    const cJSON* scene;
    i = 0;
    cJSON_ArrayForEach(scene, scenes) {
        gltf->scenes[i].id = i;
        gltf_load_scene(gltf, scene, &gltf->scenes[i]);
        i++;
    }

    const cJSON* texture;
    i = 0;
    cJSON_ArrayForEach(texture, textures) {
        gltf->textures[i].id = i;
        gltf_load_texture(gltf, texture, &gltf->textures[i]);
        i++;
    }

    return gltf;
}

void gltf_unload(gltf_gltf* gltf) {
    free(gltf->accessors);
    free(gltf->bufferViews);
    free(gltf->images);
    free(gltf->materials);
    free(gltf->samplers);
    free(gltf->textures);

    for (u32 i = 0; i < gltf->numBuffers; i++) {
        free(gltf->buffers[i].data);
    }
    free(gltf->buffers);

    for (u32 i = 0; i < gltf->numMeshes; i++) {
        free(gltf->meshes[i].primitives);
        free(gltf->meshes[i].weights);
    }
    free(gltf->meshes);

    for (u32 i = 0; i < gltf->numNodes; i++) {
        free(gltf->nodes[i].children);
        free(gltf->nodes[i].weights);
    }
    free(gltf->nodes);


    for (u32 i = 0; i < gltf->numScenes; i++) {
        free(gltf->scenes[i].nodes);
    }
    free(gltf->scenes);

    cJSON_Delete(gltf->json);

    free(gltf);
}

size_t gltf_get_accessor_offset(gltf_accessor* accessor) {
    return accessor->byteOffset + accessor->bufferView->byteOffset;
}