#version 460 core

out vec4 FragColor;

in vec3 ambientColor;

void main() {
   FragColor = vec4(ambientColor, 1.0f);
}