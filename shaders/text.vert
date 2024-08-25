#version 450

layout(location = 0) in vec2 pos0;
layout(location = 1) in vec2 pos1;
layout(location = 2) in float offset;
layout(location = 0) out vec2 fragTexCoord;

const vec2[4] vertices = vec2[4](vec2(-1.0, -1.0), vec2(1.0, -1.0), vec2(1.0, 1.0), vec2(-1.0, 1.0));
const vec2[4] uvs = vec2[4](vec2(0.0, 0.0), vec2(1.0, 0.0), vec2(1.0, 1.0), vec2(0.0, 1.0));
const vec2 resolution = vec2(800, 600);

void main() {
    //gl_Position = ubo.proj * ubo.view * ubo.model * vec4(vertices, 0.0, 1.0);
    vec2 halfSize = (pos1 - pos0) / 2;
    vec2 center = (pos0 + pos1) / 2;
    vec2 position = center + (vertices[gl_VertexIndex] * halfSize);
    float xPosScaled = position.x / resolution.x;
    float yPosScaled = position.y / resolution.y;
    gl_Position = vec4(2 * xPosScaled - 1, 2 * yPosScaled - 1, 0.0, 1.0);
    vec2 size = pos1 - pos0;
    fragTexCoord = uvs[gl_VertexIndex] * size + vec2(offset, 0);

}