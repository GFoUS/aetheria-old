#version 450

layout(binding = 0) uniform GlobalData {
    mat4 view;
    mat4 proj;
} global;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;

void main() {
    gl_Position = global.proj * global.view * vec4(inPosition, 1.0);
}