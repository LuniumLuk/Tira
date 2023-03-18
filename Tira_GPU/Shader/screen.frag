#version 460 core

out vec4 FragColor;

layout (binding = 0) uniform sampler2D uStore;
layout (binding = 1) uniform sampler2D uSampleCounter;

in vec2 TexCoords;

const float GAMMA = 2.2;
const float ONE_DIV_GAMMA = 1 / GAMMA;

vec3 reinhard_tone_mapping(vec3 color) {
    return vec3(
        color.r / (color.r + 1),
        color.g / (color.g + 1),
        color.b / (color.b + 1));
}

vec3 gamma_correction(vec3 color) {
    return vec3(
        pow(color.r, ONE_DIV_GAMMA),
        pow(color.g, ONE_DIV_GAMMA),
        pow(color.b, ONE_DIV_GAMMA));
}

void main() {
    vec3 color = texture(uStore, TexCoords).rgb * texture(uSampleCounter, TexCoords).r;
    // color = reinhard_tone_mapping(color);
    color = gamma_correction(color);

    FragColor = vec4(color, 1.0);
}