#version 450

layout(location = 0) in vec2 pos0;
layout(location = 1) in vec2 pos1;
layout(location = 2) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

vec2[4] vertices = vec2[4](vec2(-1.0, -1.0), vec2(1.0, -1.0), vec2(1.0, 1.0), vec2(-1.0, 1.0));

void main() {
    //gl_Position = ubo.proj * ubo.view * ubo.model * vec4(vertices, 0.0, 1.0);
    vec2 halfSize = (pos1 - pos0) / 2;
    vec2 center = (pos0 + pos1) / 2;
    vec2 position = center + (vertices[gl_VertexIndex] * halfSize);
    gl_Position = vec4(position, 0.0, 1.0);
    fragColor = inColor;
}
