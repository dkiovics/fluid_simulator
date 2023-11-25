#version 460 core

layout (location = 0) in vec3 pos;
layout (location = 10) in vec3 offset;

uniform mat4 M;

void main() {
	gl_Position = M * vec4(pos + offset, 1.0);
}

