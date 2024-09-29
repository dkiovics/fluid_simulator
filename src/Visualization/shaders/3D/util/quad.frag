#version 460 core
precision highp float;

out vec4 FragColor;

in vec2 texCoord;

uniform sampler2D colorTexture;

void main() {
   FragColor = texture(colorTexture, texCoord);
}