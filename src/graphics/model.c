#include "model.h"

typedef enum {
    SCALAR = 0,
    VEC2,
    VEC3,
    VEC4,
    MAT2,
    MAT3,
    MAT4
} _accessor_data_type;

size_t _get_data_type_size(_accessor_data_type type) {
    switch (type) {
        case SCALAR : return 1;
        case VEC2   : return 2;
        case VEC3   : return 3;
        case VEC4   : return 4;
        case MAT2   : return 4;
        case MAT3   : return 9;
        case MAT4   : return 16;
    }

    FATAL("Invalid accessor data type: %d", type);
    return 0;
}

_accessor_data_type _accessor_data_type_from_string(const char* string) {
    if (strcmp(string, "SCALAR") == 0) return SCALAR;
    if (strcmp(string, "VEC2") == 0)   return VEC2;
    if (strcmp(string, "VEC3") == 0)   return VEC3;
    if (strcmp(string, "VEC4") == 0)   return VEC4;
    if (strcmp(string, "MAT2") == 0)   return MAT2;
    if (strcmp(string, "MAT3") == 0)   return MAT3;
    if (strcmp(string, "MAT4") == 0)   return MAT4;

    FATAL("Invalid accessor data type: %s", string);
    return SCALAR;
}

typedef enum {
    BYTE = 5120,
    UNSIGNED_BYTE = 5121,
    SHORT = 5122,
    UNSIGNED_SHORT = 5123,
    UNSIGNED_INT = 5125,
    FLOAT = 5126
} _accessor_component_type;

size_t _get_component_type_size(_accessor_component_type componentType) {
    switch (componentType) {
    case BYTE           : return sizeof(i8);
    case UNSIGNED_BYTE  : return sizeof(u8);
    case SHORT          : return sizeof(i16);
    case UNSIGNED_SHORT : return sizeof(u16);
    case UNSIGNED_INT   : return sizeof(u32);
    case FLOAT          : return sizeof(float);
    }

    FATAL("Invalid accessor component type: %d", componentType);
    return 0;
}

typedef struct {
    size_t size;
    void* data;

    u32 count;
    size_t stride;
    _accessor_data_type type;
    _accessor_component_type componentType;
} _accessor_data;

char* _get_path_from_uri(const char* uri, const char* basePath) {
    size_t uriLength = strlen(uri);
    char* lastSlash = strrchr(basePath, (int)"/"[0]);
    u32 lastSlashPos = lastSlash - basePath;

    char* bufferFilePath = malloc(lastSlashPos + 1 + uriLength + 1); // lastSlashPos + 1 is the path up to and including the last slash, uriLength will be needed for the new path, and the final + 1 is for the null-termination character 
    CLEAR_MEMORY_ARRAY(bufferFilePath, lastSlashPos + 1 + uriLength + 1);
    memcpy(bufferFilePath, basePath, lastSlashPos + 1);
    memcpy(&bufferFilePath[lastSlashPos + 1], uri, uriLength);

    return bufferFilePath;
}

_accessor_data _get_accessor_data(model* m, u32 accessorIndex) {
    _accessor_data data;
    CLEAR_MEMORY(&data);

    size_t offset = 0;

    const cJSON* accessor = cJSON_GetArrayItem(cJSON_GetObjectItemCaseSensitive(m->data, "accessors"), accessorIndex);

    data.count = cJSON_GetObjectItemCaseSensitive(accessor, "count")->valueint;
    data.type = _accessor_data_type_from_string(cJSON_GetObjectItemCaseSensitive(accessor, "type")->valuestring);
    data.componentType = cJSON_GetObjectItemCaseSensitive(accessor, "componentType")->valueint;
    data.stride = _get_data_type_size(data.type) * _get_component_type_size(data.componentType);
    data.size = data.count * data.stride;
    data.data = malloc(data.size);
    const cJSON* accessorByteOffset = cJSON_GetObjectItemCaseSensitive(accessor, "byteOffset");
    offset += accessorByteOffset != NULL ? accessorByteOffset->valueint : 0;

    const cJSON* bufferView = cJSON_GetArrayItem(cJSON_GetObjectItemCaseSensitive(m->data, "bufferViews"), cJSON_GetObjectItemCaseSensitive(accessor, "bufferView")->valueint);

    offset += cJSON_GetObjectItemCaseSensitive(bufferView, "byteOffset")->valueint;

    const cJSON* buffer = cJSON_GetArrayItem(cJSON_GetObjectItemCaseSensitive(m->data, "buffers"), cJSON_GetObjectItemCaseSensitive(bufferView, "buffer")->valueint);

    const char* uri = cJSON_GetObjectItemCaseSensitive(buffer, "uri")->valuestring;
    char* bufferFilePath = _get_path_from_uri(uri, m->path);

    FILE* bufferFile = fopen(bufferFilePath, "rb");
    if (bufferFile == NULL) {
        FATAL("Failed to open file: %s", bufferFilePath);
    }
    free(bufferFilePath);

    fseek(bufferFile, offset, SEEK_SET);
    fread(data.data, data.size, 1, bufferFile);
    fclose(bufferFile);

    return data;
}

void _load_mesh(const cJSON* meshData, model_mesh* mesh) {
    u32 materialID = cJSON_GetObjectItemCaseSensitive(meshData, "material")->valueint;
    mesh->material = &mesh->m->materials[materialID];

    const cJSON* attributes = cJSON_GetObjectItemCaseSensitive(meshData, "attributes");

    u32 indicesAccessorIndex = cJSON_GetObjectItemCaseSensitive(meshData, "indices")->valueint;
    u32 positionAccesssorIndex = cJSON_GetObjectItemCaseSensitive(attributes, "POSITION")->valueint;
    u32 normalAccessorIndex = cJSON_GetObjectItemCaseSensitive(attributes, "NORMAL")->valueint; 

    _accessor_data indicesData = _get_accessor_data(mesh->m, indicesAccessorIndex);
    _accessor_data positionData = _get_accessor_data(mesh->m, positionAccesssorIndex);
    _accessor_data normalData = _get_accessor_data(mesh->m, normalAccessorIndex);

    mesh->numIndices = indicesData.count;
    mesh->indexBuffer = vulkan_buffer_create_with_data(mesh->m->ctx, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, indicesData.size, indicesData.data);
    free(indicesData.data);

    mesh->numBuffers = 3;
    mesh->vertexBuffers = malloc(sizeof(vulkan_buffer*) * mesh->numBuffers);
    mesh->vertexBuffers[0] = vulkan_buffer_create_with_data(mesh->m->ctx, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, positionData.size, positionData.data);
    mesh->vertexBuffers[1] = vulkan_buffer_create_with_data(mesh->m->ctx, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, normalData.size, normalData.data);
    free(positionData.data);
    free(normalData.data);

    if (!mesh->material->baseColorTexture.defaultTexture) {
        char texCoordBase[9] = "TEXCOORD_";
        char baseColorTextureTexCoord[14];
        memcpy(baseColorTextureTexCoord, texCoordBase, 9);
        sprintf(&baseColorTextureTexCoord[9], "%d", mesh->material->baseColorTexture.texCoord);
        u32 colorUVAccessorIndex = cJSON_GetObjectItemCaseSensitive(attributes, baseColorTextureTexCoord)->valueint;
        _accessor_data colorUVData = _get_accessor_data(mesh->m, colorUVAccessorIndex);
        INFO("%f", ((float*)colorUVData.data)[0]);
        mesh->vertexBuffers[2] = vulkan_buffer_create_with_data(mesh->m->ctx, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, colorUVData.size, colorUVData.data);
        free(colorUVData.data);
    } else {
        vec2* colorUVs = malloc(sizeof(vec2) * positionData.count);
        CLEAR_MEMORY_ARRAY(colorUVs, positionData.count);
        mesh->vertexBuffers[2] = vulkan_buffer_create_with_data(mesh->m->ctx, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, sizeof(vec2) * positionData.count, colorUVs);
        free(colorUVs);
    }
}

void _load_node(const cJSON* nodeData, model_node* node) {
    const cJSON* meshID = cJSON_GetObjectItemCaseSensitive(nodeData, "mesh");
    if (meshID != NULL) {
        const cJSON* mesh = cJSON_GetArrayItem(cJSON_GetObjectItemCaseSensitive(node->m->data, "meshes"), meshID->valueint);
        u32 offset = 0;
        for (u32 i = 0; i < meshID->valueint; i++) {
            offset += node->m->meshPrimitivesLookup[i];
        }

        const cJSON* primitives = cJSON_GetObjectItemCaseSensitive(mesh, "primitives");
        node->numMeshes = cJSON_GetArraySize(primitives);
        node->meshes = malloc(sizeof(model_mesh*) * node->numMeshes);
        for (u32 i = 0; i < node->numMeshes; i++) {
            node->meshes[i] = &node->m->meshes[offset + i];
        }
    }

    const cJSON* children = cJSON_GetObjectItemCaseSensitive(nodeData, "children");
    node->numChildren = cJSON_GetArraySize(children);
    node->children = malloc(sizeof(model_node*) * node->numChildren);
    CLEAR_MEMORY_ARRAY(node->children, node->numChildren);

    u32 i = 0;
    const cJSON* childID;
    cJSON_ArrayForEach(childID, children) {
        node->children[i] = &node->m->nodes[childID->valueint];
        i++;
    }
}

void _load_material(const cJSON* materialData, model_material* material) {
    const cJSON* pbr = cJSON_GetObjectItemCaseSensitive(materialData, "pbrMetallicRoughness");
    
    const cJSON* baseColorFactorObj = cJSON_GetObjectItemCaseSensitive(pbr, "baseColorFactor");
    if (baseColorFactorObj != NULL) {
        const cJSON* color;
        u32 i = 0;
        cJSON_ArrayForEach(color, baseColorFactorObj) {
            material->data.baseColorFactor[i++] = (float)color->valuedouble; 
        }
    } else {
        vec4 white = {1.0f, 1.0f, 1.0f, 1.0f};
        memcpy(&material->data.baseColorFactor, &white, sizeof(vec4));
    }

    const cJSON* baseColorTexture = cJSON_GetObjectItemCaseSensitive(pbr, "baseColorTexture");
    if (baseColorTexture != NULL) {
        u32 textureIndex = cJSON_GetObjectItemCaseSensitive(baseColorTexture, "index")->valueint;
        const cJSON* textureData = cJSON_GetArrayItem(cJSON_GetObjectItemCaseSensitive(material->m->data, "textures"), textureIndex);
        material->baseColorTexture.image = material->m->images[cJSON_GetObjectItemCaseSensitive(textureData, "source")->valueint];
        material->baseColorTexture.defaultTexture = false;
        
        const cJSON* texCoord = cJSON_GetObjectItemCaseSensitive(baseColorTexture, "texCoord");
        if (texCoord != NULL) {
            material->baseColorTexture.texCoord = texCoord->valueint;
        } else {
            material->baseColorTexture.texCoord = 0;
        }
    } else {
        material->baseColorTexture.image = vulkan_image_get_default_color_texture(material->m->ctx);
        material->baseColorTexture.texCoord = 0;
        material->baseColorTexture.defaultTexture = true;
    }
    vulkan_descriptor_set_write_image(material->materialDataSet, 1, material->baseColorTexture.image);

    material->materialDataBuffer = vulkan_buffer_create_with_data(material->m->ctx, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(model_material_data), &material->data);
    vulkan_descriptor_set_write_buffer(material->materialDataSet, 0, material->materialDataBuffer);
}

model* model_load_from_file(const char* path, vulkan_context* ctx, vulkan_descriptor_set_layout* materialSetLayout) {
    model* this = malloc(sizeof(model));
    this->ctx = ctx;
    this->path = path;
    this->materialSetAllocator = vulkan_descriptor_allocator_create(ctx->device, materialSetLayout);
    
    FILE* modelFile = fopen(path, "r");
    if (modelFile == NULL) {
        FATAL("Failed to open model file: %s", modelFile);
    }
    fseek(modelFile, 0L, SEEK_END);
    u64 modelFileSize = ftell(modelFile);
    fseek(modelFile, 0L, SEEK_SET);
    char* modelText = malloc(modelFileSize);
    fread(modelText, modelFileSize, 1, modelFile);
    fclose(modelFile);

    this->data = cJSON_Parse(modelText);
    free(modelText);

    // Allocate mesh arrays and node arrays early so pointers can be created
    const cJSON* meshes = cJSON_GetObjectItemCaseSensitive(this->data, "meshes");

    this->meshPrimitivesLookup = malloc(sizeof(u32) * cJSON_GetArraySize(meshes));
    CLEAR_MEMORY_ARRAY(this->meshPrimitivesLookup, cJSON_GetArraySize(meshes));

    const cJSON* mesh;
    u32 totalPrimitives = 0;
    u32 i = 0;
    cJSON_ArrayForEach(mesh, meshes) {
        const cJSON* primitives = cJSON_GetObjectItemCaseSensitive(mesh, "primitives");
        this->meshPrimitivesLookup[i] = cJSON_GetArraySize(primitives);
        totalPrimitives += cJSON_GetArraySize(primitives);
        i++;
    }

    this->numMeshes = totalPrimitives;
    this->meshes = malloc(sizeof(model_mesh) * this->numMeshes);
    CLEAR_MEMORY_ARRAY(this->meshes, this->numMeshes);

    const cJSON* nodes = cJSON_GetObjectItemCaseSensitive(this->data, "nodes");
    this->numNodes = cJSON_GetArraySize(nodes);
    this->nodes = malloc(sizeof(model_node) * this->numNodes);
    CLEAR_MEMORY_ARRAY(this->nodes, this->numNodes);

    const cJSON* materials = cJSON_GetObjectItemCaseSensitive(this->data, "materials");
    this->numMaterials = cJSON_GetArraySize(materials);
    this->materials = malloc(sizeof(model_material) * this->numMaterials);
    CLEAR_MEMORY_ARRAY(this->materials, this->numMaterials);

    const cJSON* images = cJSON_GetObjectItemCaseSensitive(this->data, "images");
    this->numImages = cJSON_GetArraySize(images);
    this->images = malloc(sizeof(vulkan_image*) * this->numImages);
    CLEAR_MEMORY_ARRAY(this->images, this->numImages);

    // Load images
    i = 0;
    const cJSON* image;
    cJSON_ArrayForEach(image, images) {
        const char* uri = cJSON_GetObjectItemCaseSensitive(image, "uri")->valuestring;
        char* imageFilePath = _get_path_from_uri(uri, this->path);
        this->images[i] = vulkan_image_create_from_file(this->ctx, imageFilePath, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
        i++;
    }

    // Load materials
    i = 0;
    const cJSON* material;
    cJSON_ArrayForEach(material, materials) {
        this->materials[i].id = i;
        this->materials[i].m = this;
        this->materials[i].materialDataSet = vulkan_descriptor_set_allocate(this->materialSetAllocator);
        _load_material(material, &this->materials[i]);
        i++;
    }

    // Load meshes
    i = 0;
    cJSON_ArrayForEach(mesh, meshes) {
        const cJSON* primitives = cJSON_GetObjectItemCaseSensitive(mesh, "primitives");
        const cJSON* primitive;
        cJSON_ArrayForEach(primitive, primitives) {
            this->meshes[i].id = i;
            this->meshes[i].m = this;
            _load_mesh(primitive, &this->meshes[i]);
            i++;
        }
    }

    // Load nodes
    i = 0;
    const cJSON* nodeData = NULL;
    cJSON_ArrayForEach(nodeData, nodes) {
        this->nodes[i].id = i;
        this->nodes[i].m = this;
        _load_node(nodeData, &this->nodes[i]);
        i++;
    }

    // Fetch root nodes
    const cJSON* scene = cJSON_GetArrayItem(cJSON_GetObjectItemCaseSensitive(this->data, "scenes"), cJSON_GetObjectItemCaseSensitive(this->data, "scene")->valueint);
    const cJSON* rootNodes = cJSON_GetObjectItemCaseSensitive(scene, "nodes");
    this->numRootNodes = cJSON_GetArraySize(rootNodes);
    this->rootNodes = malloc(sizeof(model_node*) * this->numRootNodes);
    CLEAR_MEMORY_ARRAY(this->rootNodes, this->numRootNodes);
    
    i = 0;
    const cJSON* rootNode;
    cJSON_ArrayForEach(rootNode, rootNodes) {
        this->rootNodes[i] = &this->nodes[rootNode->valueint];
        i++;
    }

    return this;
}
void model_destroy(model* this) {
    for (u32 i = 0; i < this->numMeshes; i++) {
        model_mesh* mesh = &this->meshes[i];

        vulkan_buffer_destroy(mesh->indexBuffer);
        for (u32 j = 0; j < mesh->numBuffers; j++) {
            vulkan_buffer_destroy(mesh->vertexBuffers[j]);
        }
        free(mesh->vertexBuffers);
    }
    free(this->meshes);

    for (u32 i = 0; i < this->numNodes; i++) {
        free(this->nodes[i].children);
        free(this->nodes[i].meshes);
    }
    free(this->nodes);
    free(this->rootNodes);
    free(this->meshPrimitivesLookup);
    vulkan_descriptor_allocator_destroy(this->materialSetAllocator);

    cJSON_Delete(this->data);

    free(this);
}

void _render_node(model_node* node, VkCommandBuffer cmd, VkPipelineLayout layout) {
    for (u32 i = 0; i < node->numMeshes; i++) {
        model_mesh* mesh = node->meshes[i];
        VkBuffer* buffers = malloc(sizeof(VkBuffer) * mesh->numBuffers);
        for (u32 j = 0; j < mesh->numBuffers; j++) {
            buffers[j] = mesh->vertexBuffers[j]->buffer;
        }
        VkDeviceSize* offsets = malloc(sizeof(VkDeviceSize) * mesh->numBuffers);
        CLEAR_MEMORY_ARRAY(offsets, mesh->numBuffers);

        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 1, 1, &mesh->material->materialDataSet->set, 0, NULL);
        vkCmdBindVertexBuffers(cmd, 0, mesh->numBuffers, buffers, offsets);
        free(buffers);
        free(offsets);


        switch (mesh->indexBuffer->size / mesh->numIndices) {
        case(sizeof(u16)) : {
            vkCmdBindIndexBuffer(cmd, mesh->indexBuffer->buffer, 0, VK_INDEX_TYPE_UINT16);
            break;
        }
        case(sizeof(u32)) : {
            vkCmdBindIndexBuffer(cmd, mesh->indexBuffer->buffer, 0, VK_INDEX_TYPE_UINT32);
            break;
        }
        }

        vkCmdDrawIndexed(cmd, mesh->numIndices, 1, 0, 0, 0);
    }

    for (u32 i = 0; i < node->numChildren; i++) {
        _render_node(node->children[i], cmd, layout);
    }
}

void model_render(model* m, VkCommandBuffer cmd, VkPipelineLayout layout) {
    for (u32 i = 0; i < m->numRootNodes; i++) {
        _render_node(m->rootNodes[i], cmd, layout);
    }
}