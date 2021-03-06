#version 450

layout(set = 0, binding = 0) uniform GlobalData {
    mat4 view;
    mat4 proj;
} global;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inColorUV;

layout(location = 0) out vec2 fragColorUV;
layout(location = 1) out vec3 fragPosition;

void main() {
    gl_Position = global.proj * global.view * vec4(inPosition, 1.0);
    fragColorUV = inColorUV;
    fragPosition = gl_Position.xyz;
}