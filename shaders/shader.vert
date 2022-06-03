#version 450

layout(binding = 0) uniform GlobalData {
    mat4 view;
    mat4 proj;
} global;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUV;

layout(location = 0) out vec2 fragUV;

vec3 colors[4] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0),
    vec3(1.0, 1.0, 0.0)
);

void main() {
    gl_Position = global.proj * global.view * vec4(inPosition, 1.0);
    fragUV = inUV;
}