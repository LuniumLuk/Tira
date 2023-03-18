#version 460 core

out vec4 FragColor;

layout (binding = 0) uniform sampler2D uRenderTarget;
layout (binding = 1) uniform sampler2D uTransfer;

in vec2 TexCoords;

void main() {
    // Mix masked area and preserve unmasked area.
    FragColor = texture(uRenderTarget, TexCoords) + texture(uTransfer, TexCoords);
}