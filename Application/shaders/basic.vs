#version 460 core

layout (location = 0) in vec3 pos;
layout (location = 10) in vec3 offset;
layout (location = 11) in vec3 color;

uniform mat4 M;

uniform float scale;

out vec3 ambientColor;

void main() {
	vec3 scaledPos = pos * scale;
	gl_Position = M * vec4(scaledPos + offset, 1.0);
	ambientColor = color;
}