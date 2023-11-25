#version 460 core

out vec4 FragColor;

in vec3 ambientColor;
in vec2 texCoord;

uniform sampler2D texture0;

void main() {
   FragColor = vec4(texture(texture0, texCoord).xyz, 1.0f);
}