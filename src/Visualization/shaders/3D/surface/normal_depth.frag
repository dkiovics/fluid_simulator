#version 460 core
precision highp float;

out vec4 normalAndDepth;

in vec2 texCoord;

uniform sampler2D depthTexture;

uniform struct{
    mat4 viewMatrix;
	mat4 viewMatrixInverse;
	mat4 projectionMatrix;
	mat4 projectionMatrixInverse;
    vec4 position;
} camera;


vec3 uvToEye(vec2 texCoord, float depth) {
	vec4 ndc = vec4(texCoord * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
	vec4 eyeSpacePos = camera.projectionMatrixInverse * ndc;
	return eyeSpacePos.xyz / eyeSpacePos.w;
}

vec3 getEyePos(sampler2D depthTexture, vec2 texCoord) {
	return uvToEye(texCoord, texture(depthTexture, texCoord).x);
}

void main(){
	float depth = texture(depthTexture, texCoord).x;
	if (depth == 1.0f) {
		discard;
		return;
	}
	
	vec3 posEye = uvToEye(texCoord, depth);
	vec2 texelSize = vec2(1.0, 1.0) / textureSize(depthTexture, 0);
	
	vec3 ddx = getEyePos(depthTexture, texCoord + vec2(texelSize.x, 0)) - posEye;
	vec3 ddx2 = posEye - getEyePos(depthTexture, texCoord + vec2(-texelSize.x, 0));
	if (abs(ddx.z) > abs(ddx2.z)) {
		ddx = ddx2;
	}
	vec3 ddy = getEyePos(depthTexture, texCoord + vec2(0, texelSize.y)) - posEye;
	vec3 ddy2 = posEye - getEyePos(depthTexture, texCoord + vec2(0, -texelSize.y));
	if (abs(ddy2.z) < abs(ddy.z)) {
		ddy = ddy2;
	}
	
	vec3 normal = (camera.viewMatrixInverse * vec4(normalize(cross(ddx, ddy)), 0.0)).xyz;
	
	normalAndDepth = vec4(normal, depth);
}