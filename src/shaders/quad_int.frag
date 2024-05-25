#version 460 core
precision highp float;

out vec4 FragColor;

in vec2 texCoord;

uniform usampler2D colorTexture;

void main() {
   uvec4 c = texture(colorTexture, texCoord);
   FragColor = vec4(c) / 40000.0;
}