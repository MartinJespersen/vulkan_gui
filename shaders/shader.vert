#version 450

layout(location = 0) in vec2 pos0;
layout(location = 1) in vec2 pos1;
layout(location = 2) in vec3 inColor;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec2 outCenter;
layout(location = 3) out vec2 outHalfSize;
layout(location = 4) out vec2 outPos;
layout(push_constant) uniform PushConstants {
    vec2 resolution;
} pushConstants;

const vec2[4] vertices = vec2[4](vec2(-1.0, -1.0), vec2(1.0, -1.0), vec2(1.0, 1.0), vec2(-1.0, 1.0));
const vec2[4] uvs = vec2[4](vec2(0.0, 0.0), vec2(1.0, 0.0), vec2(1.0, 1.0), vec2(0.0, 1.0));

void main() {
    //gl_Position = ubo.proj * ubo.view * ubo.model * vec4(vertices, 0.0, 1.0);
    vec2 halfSize = (pos1 - pos0) / 2;
    vec2 center = (pos0 + pos1) / 2;
    vec2 position = center + (vertices[gl_VertexIndex] * halfSize);
    gl_Position = vec4(2 * position.x - 1, 2 * position.y - 1, 0.0, 1.0);

    outPos = position * pushConstants.resolution;
    outCenter = center * pushConstants.resolution;
    outHalfSize = halfSize * pushConstants.resolution;
    fragColor = inColor;
}
