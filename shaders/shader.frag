#version 450

layout(location = 0) in vec3 fragColor; // Input texture coordinates
layout(location = 1) in vec2 inCenter;
layout(location = 2) in vec2 inHalfSize;
layout(location = 3) in vec2 inPos;
layout(location = 4) in float softness;
layout(location = 5) in float borderThickness;
layout(location = 6) in float cornerRadius;

layout(location = 0) out vec4 outColor;    // Output color

float borderFactor = 1.0;

float RoundedRectSDF(
    vec2 sample_pos,
    vec2 rect_center,
    vec2 rect_half_size,
    float r
) {
    vec2 d2 = (abs(rect_center - sample_pos) -
        rect_half_size +
        vec2(r, r));
    return min(max(d2.x, d2.y), 0.0) + length(max(d2, 0.0)) - r;
}
void main() {

    vec2 softnessPadding = vec2(max(0, softness * 2 - 1), max(0, softness * 2 - 1));
    if(borderThickness != 0.0) {
        vec2 interiorHalfSize = inHalfSize - vec2(borderThickness);
        float interiorRadiusReduceF = min(interiorHalfSize.x / inHalfSize.x, interiorHalfSize.y / inHalfSize.y);
        float interiorCornerRadius = (cornerRadius *
            interiorRadiusReduceF *
            interiorRadiusReduceF);
        float insideD = RoundedRectSDF(inPos, inCenter, interiorHalfSize - softnessPadding, interiorCornerRadius);
        float insideF = 1.0 - smoothstep(0.0, 2 * softness, insideD);
        borderFactor = insideF;
    }

    float dist = RoundedRectSDF(inPos, inCenter, inHalfSize - softnessPadding, cornerRadius);
    float sdfFactor = 1.0 - smoothstep(0.0, 2 * softness, dist);
    outColor = vec4(fragColor * sdfFactor * borderFactor, 1.0);
}
