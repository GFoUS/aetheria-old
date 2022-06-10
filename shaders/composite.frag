#version 450

layout(input_attachment_index = 0, set = 0, binding = 0) uniform subpassInputMS accum;
layout(input_attachment_index = 1, set = 0, binding = 1) uniform subpassInputMS reveal;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 accum = subpassLoad(accum, gl_SampleID);
    float reveal = subpassLoad(reveal, gl_SampleID).r;

    outColor = vec4(accum.rgb / max(accum.a, 1e-5), reveal);
}

