#version 450

layout(location = 0) out vec4 outColor;

layout (set = 1, binding = 0) uniform ModelData {
    vec4 baseColorFactor;
} modelData;

layout (set = 1, binding = 1) uniform sampler2D baseColorTexture;

layout(location = 2) in vec2 fragColorUV;

void main() {
    outColor = modelData.baseColorFactor * vec4(texture(baseColorTexture, fragColorUV).rgb, 1.0);
}