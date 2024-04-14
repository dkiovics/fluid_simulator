#version 460 core
precision highp float;

layout (location = 0) in vec4 pos;
layout (location = 2) in vec2 texCoordIn;

out vec2 texCoord;

void main() {
	gl_Position = vec4(pos.xyz * 2.0, 1);
	texCoord = texCoordIn;
}