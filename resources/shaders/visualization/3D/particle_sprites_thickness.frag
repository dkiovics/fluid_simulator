#version 460 core
precision highp float;

layout (location = 0) out vec4 thickness;

uniform sampler2D depthTexture;

in vec2 texCoord;

uniform struct{
    mat4 viewMatrix;
	mat4 viewMatrixInverse;
	mat4 projectionMatrix;
	mat4 projectionMatrixInverse;
    vec4 position;
} camera;

uniform float particleRadius;

void main(void) {
	vec2 xy = texCoord*2.0-1.0;
	float r2 = dot(xy, xy);
	if (r2 > 1.0){
		discard;
		return;
	}
	thickness.r = sqrt(1.0 - r2) * particleRadius;
}
