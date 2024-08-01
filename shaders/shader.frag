#version 450

layout(location = 0) in vec3 fragColor; // Input texture coordinates
layout(location = 0) out vec4 outColor;    // Output color

void main() {
    outColor = vec4(fragColor, 1.0); // Set the output color with full opacity
}
