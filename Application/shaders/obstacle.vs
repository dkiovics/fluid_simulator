#version 460 core

layout (location = 0) in vec3 pos;
layout (location = 2) in vec2 texCoordIn;
layout (location = 10) in vec3 offset;
layout (location = 11) in vec3 color;

uniform mat4 M;

out vec2 texCoord;

void main() {
	gl_Position = M * vec4(pos + offset, 1.0);
	texCoord = texCoordIn;
}