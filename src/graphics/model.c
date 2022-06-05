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
    if (strcmp(string, "SCALAR")) return SCALAR;
    if (strcmp(string, "VEC2"))   return VEC2;
    if (strcmp(string, "VEC3"))   return VEC3;
    if (strcmp(string, "VEC4"))   return VEC4;
    if (strcmp(string, "MAT2"))   return MAT2;
    if (strcmp(string, "MAT3"))   return MAT3;
    if (strcmp(string, "MAT4"))   return MAT4;

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
    offset += cJSON_GetObjectItemCaseSensitive(accessor, "byteOffset")->valueint;

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

    // TODO: Load buffer into memory and send to GPU

    return data;
}

void _load_mesh(const cJSON* meshData, model_mesh* mesh) {
    const cJSON* primitives = cJSON_GetObjectItemCaseSensitive(meshData, "primitives");
    const cJSON* primitive = cJSON_GetArrayItem(primitives, 0);

    const cJSON* attributes = cJSON_GetObjectItemCaseSensitive(primitive, "attributes");
    u32 indicesAccessorIndex = cJSON_GetObjectItemCaseSensitive(primitive, "indices")->valueint;

    u32 positionAccesssorIndex = cJSON_GetObjectItemCaseSensitive(attributes, "POSITION")->valueint;
    u32 normalAccessorIndex = cJSON_GetObjectItemCaseSensitive(attributes, "NORMAL"); 

    _accessor_data indicesAccessorData = _get_accessor_data(mesh->m, indicesAccessorIndex);
}

void _load_node(const cJSON* nodeData, model_node* node) {
    const cJSON* meshes = cJSON_GetObjectItemCaseSensitive(nodeData, "meshes");
    node->numMeshes = cJSON_GetArraySize(meshes);
    node->meshes = malloc(sizeof(model_mesh*) * node->numMeshes);
    CLEAR_MEMORY_ARRAY(node->meshes, node->numMeshes);

    u32 i = 0;
    const cJSON* meshID;
    cJSON_ArrayForEach(meshID, meshes) {
        node->meshes[i] = &node->m->meshes[meshID->valueint];
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

    // Load nodes
    const cJSON* nodes = cJSON_GetObjectItemCaseSensitive(this->data, "nodes");
    this->numNodes = cJSON_GetArraySize(nodes);
    this->nodes = malloc(sizeof(model_node) * this->numNodes);
    CLEAR_MEMORY_ARRAY(this->nodes, this->numNodes);

    u32 i = 0;
    const cJSON* nodeData = NULL;
    cJSON_ArrayForEach(nodeData, nodes) {
        this->nodes[i].id = i;
        this->nodes[i].m = this;
        _load_node(nodeData, &this->nodes[i]);
        i++;
    }

    // Load meshes
    const cJSON* meshes = cJSON_GetObjectItemCaseSensitive(this->data, "meshes");
    this->numMeshes = cJSON_GetArraySize(meshes);
    this->meshes = malloc(sizeof(model_mesh) * this->numMeshes);
    CLEAR_MEMORY_ARRAY(this->meshes, this->numMeshes);

    i = 0;
    const cJSON* meshData = NULL;
    cJSON_ArrayForEach(meshData, meshes) {
        this->meshes[i].id = i;
        this->meshes[i].m = this;
        _load_mesh(meshData, &this->meshes[i]);
        i++;
    }

    cJSON_Delete(this->data);

    return this;
}
void model_destroy(model* this);

void model_render(model* m);