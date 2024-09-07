#version 460 core
precision highp float;

out vec4 fragmentColor;
in vec2 texCoord;
in vec3 eyeSpacePos;
in mat3 billboardM;

uniform struct{
    mat4 viewMatrix;
	mat4 viewMatrixInverse;
	mat4 projectionMatrix;
	mat4 projectionMatrixInverse;
    vec4 position;
} camera;

uniform struct {
	vec4 diffuseColor;
    vec4 specularColor;
    float shininess;
} object;

uniform struct {
  vec4 position;		//if the w component of the position is 0 than the light is directional, otherwise its a point light
  vec3 powerDensity;
} lights[100];

uniform int lightNum;

uniform float particleRadius;

vec3 shade(vec4 worldPos, vec4 normal, vec4 viewDir, vec4 lightPos, vec3 powerDensity, vec4 materialColor) {
	vec3 frag2light = lightPos.xyz - worldPos.xyz * lightPos.w;
    vec4 lightDir = vec4(normalize(frag2light), 0);
    if(dot(normal, viewDir) < 0.0)
        normal = -normal;
    float lightCos = dot(lightDir, normal);
    if(lightCos < 0.0)
        return vec3(0.0, 0.0, 0.0);
    float d2 = length(frag2light);
    d2 = d2*d2;
    vec4 halfWay = normalize(viewDir + lightDir);
    float shine = dot(halfWay, normal);
    if(shine < 0.0)
        shine = 0.0;
    return powerDensity * (materialColor.xyz * lightCos + object.specularColor.xyz * pow(shine, object.shininess)) / d2;
}

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
	
	vec4 eyeSpacePixelPos = vec4(eyeSpacePos + normalize(eyeSpaceNormal)*particleRadius, 1.0);
	vec4 clipSpacePos = camera.projectionMatrix * eyeSpacePixelPos;
	gl_FragDepth = clipSpacePos.z / clipSpacePos.w * 0.5 + 0.5;
	
	
	
	vec4 normal = camera.viewMatrixInverse * vec4(N, 0);
	vec4 worldPosition = camera.viewMatrixInverse * eyeSpacePixelPos;
	
	fragmentColor = vec4(0.0,0.0,0.0,object.diffuseColor.a);
    for(int p = 0; p < lightNum; p++) {
        vec4 lightPositionOrDir = lights[p].position;
        vec3 powerDensity = lights[p].powerDensity;
        vec4 viewDir = normalize(camera.position - worldPosition);
		
        fragmentColor.rgb += shade(worldPosition, normal, viewDir, lightPositionOrDir, powerDensity, object.diffuseColor);
    }
}