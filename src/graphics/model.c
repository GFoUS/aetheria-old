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
    size_t uriLength = strlen(uri);
    char* lastSlash = strrchr(m->path, (int)"/"[0]);
    u32 lastSlashPos = lastSlash - m->path;

    char* bufferFilePath = malloc(lastSlashPos + 1 + uriLength + 1); // lastSlashPos + 1 is the path up to and including the last slash, uriLength will be needed for the new path, and the final + 1 is for the null-termination character 
    CLEAR_MEMORY_ARRAY(bufferFilePath, lastSlashPos + 1 + uriLength + 1);
    memcpy(bufferFilePath, m->path, lastSlashPos + 1);
    memcpy(&bufferFilePath[lastSlashPos + 1], uri, uriLength);

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

    mesh->numBuffers = 2;
    mesh->vertexBuffers = malloc(sizeof(vulkan_buffer*) * 2);
    mesh->vertexBuffers[0] = vulkan_buffer_create_with_data(mesh->m->ctx, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, positionData.size, positionData.data);
    mesh->vertexBuffers[1] = vulkan_buffer_create_with_data(mesh->m->ctx, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, normalData.size, normalData.data);
    free(positionData.data);
    free(normalData.data);
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

model* model_load_from_file(const char* path, vulkan_context* ctx) {
    model* this = malloc(sizeof(model));
    this->ctx = ctx;
    this->path = path;
    
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

    cJSON_Delete(this->data);

    free(this);
}

void _render_node(model_node* node, VkCommandBuffer cmd) {
    for (u32 i = 0; i < node->numMeshes; i++) {
        model_mesh* mesh = node->meshes[i];
        VkBuffer* buffers = malloc(sizeof(VkBuffer) * mesh->numBuffers);
        for (u32 j = 0; j < mesh->numBuffers; j++) {
            buffers[j] = mesh->vertexBuffers[j]->buffer;
        }
        VkDeviceSize* offsets = malloc(sizeof(VkDeviceSize) * mesh->numBuffers);
        CLEAR_MEMORY_ARRAY(offsets, mesh->numBuffers);

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
        _render_node(node->children[i], cmd);
    }
}

void model_render(model* m, VkCommandBuffer cmd) {
    for (u32 i = 0; i < m->numRootNodes; i++) {
        _render_node(m->rootNodes[i], cmd);
    }
}