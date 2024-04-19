#version 460 core
precision highp float;

out vec4 fragmentColor;

in vec2 texCoord;

uniform sampler2D depthTexture;
uniform sampler2D thicknessTexture;
uniform float transparency;
uniform int transparencyEnabled;

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


vec3 uvToEye(vec2 texCoord, float depth) {
	vec4 ndc = vec4(texCoord * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
	vec4 eyeSpacePos = camera.projectionMatrixInverse * ndc;
	return eyeSpacePos.xyz / eyeSpacePos.w;
}

vec3 getEyePos(sampler2D depthTexture, vec2 texCoord) {
	return uvToEye(texCoord, texture(depthTexture, texCoord).x);
}

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

void main(){
	float depth = texture(depthTexture, texCoord).x;
	vec2 texelSize = vec2(1.0, 1.0) / textureSize(depthTexture, 0);
	if (depth == 1.0f) {
		discard;
		return;
	}
	
	gl_FragDepth = depth;
	
	vec3 posEye = uvToEye(texCoord, depth);
	
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
	
	
	
	vec4 normal = vec4(cross(ddx, ddy), 0);
	normal = camera.viewMatrixInverse * normalize(normal);
	vec4 worldPosition = camera.viewMatrixInverse * vec4(posEye, 1.0);
	
	if(transparencyEnabled == 1){
		float thickness = texture(thicknessTexture, texCoord).x;
		fragmentColor = vec4(0.0, 0.0, 0.0, exp(-transparency * thickness));
	}else{
		fragmentColor = vec4(0.0, 0.0, 0.0, object.diffuseColor.a);
	}
	
    for(int p = 0; p < lightNum; p++) {
        vec4 lightPositionOrDir = lights[p].position;
        vec3 powerDensity = lights[p].powerDensity;
        vec4 viewDir = normalize(camera.position - worldPosition);
		
        fragmentColor.rgb += shade(worldPosition, normal, viewDir, lightPositionOrDir, powerDensity, object.diffuseColor);
    }
}