#version 460 core

out vec4 FragColor;

in vec4 ambientColor;

void main() {
   FragColor = ambientColor;
}