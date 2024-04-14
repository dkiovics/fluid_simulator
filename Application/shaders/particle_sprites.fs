#version 460 core
precision highp float;

//out vec4 fragmentColor;
in vec2 texCoord;
in vec3 eyeSpacePos;
in vec4 color;
in mat3 billboardM;

uniform struct{
    mat4 viewMatrix;
	mat4 projectionMatrix;
    vec4 position;
} camera;

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
	
	vec3 eyeSpaceNormal = N * billboardM;
	
	vec4 pixelPos = vec4(eyeSpacePos + eyeSpaceNormal*particleRadius, 1.0);
	vec4 clipSpacePos = camera.projectionMatrix * pixelPos;
	gl_FragDepth = clipSpacePos.z / clipSpacePos.w * 0.5 + 0.5;
	
	//vec3 lightDir = normalize(vec3(1, 1, -1));
	
	//fragmentColor = vec4(color.xyz * max(0.0, dot(N, lightDir)), color.a);
	//fragmentColor = vec4(gl_FragDepth, gl_FragDepth, gl_FragDepth, color.a);
}