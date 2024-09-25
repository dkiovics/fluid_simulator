#version 460 core
precision highp float;

layout(location = 0) out ivec4 geometryId;

//out vec4 fragmentColor;
in vec2 texCoord;
in vec3 eyeSpacePos;
//in vec4 color;
in mat3 billboardM;
flat in unsigned int instanceID;

uniform ivec2 resolution;

uniform struct{
    mat4 viewMatrix;
	mat4 viewMatrixInverse;
	mat4 projectionMatrix;
	mat4 projectionMatrixInverse;
    vec4 position;
} camera;

struct FragmentParam {
	int paramNum;
	int paramIndexes[30];
};

layout(std430, binding = 20) restrict writeonly buffer pixelParamsSSBO {
	FragmentParam pixelParams[];
};

uniform float particleRadius;

void main(void) {
	vec3 N;
	N.xy = texCoord*2.0-1.0;
	float r2 = dot(N.xy, N.xy);
	if (r2 > 1.0){
		discard;
		return;
	}
	N.z = -sqrt(1.0 - r2);
	
	vec3 eyeSpaceNormal = billboardM * N;
	
	vec4 pixelPos = vec4(eyeSpacePos + normalize(eyeSpaceNormal)*particleRadius, 1.0);
	vec4 clipSpacePos = camera.projectionMatrix * pixelPos;
	gl_FragDepth = clipSpacePos.z / clipSpacePos.w * 0.5 + 0.5;

	int index = int(gl_FragCoord.y) * resolution.x + int(gl_FragCoord.x);
	pixelParams[index].paramIndexes[0] = int(instanceID);
	pixelParams[index].paramNum = 1;

	geometryId = ivec4(instanceID, 0, 0, 0);
	
	//vec3 lightDir = normalize(vec3(1, 1, -1));
	
	//fragmentColor = vec4(color.xyz * max(0.0, dot(N, lightDir)), color.a);
	//fragmentColor = vec4(gl_FragDepth, gl_FragDepth, gl_FragDepth, color.a);
}