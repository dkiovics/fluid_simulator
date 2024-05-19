#version 460 core
precision highp float;

out vec4 fragmentColor;
in vec2 textureCoords;
in vec4 worldPosition;
in vec4 worldNormal;
in vec4 diffuseColor;

uniform struct {
    mat4 modelMatrix;
	mat4 modelMatrixInverse;
    vec4 specularColor;
    float shininess;
} object;

uniform struct{
    mat4 viewMatrix;
	mat4 projectionMatrix;
    vec4 position;
} camera;

uniform struct {
  vec4 position;		//if the w component of the position is 0 than the light is directional, otherwise its a point light
  vec3 powerDensity;
} lights[100];

uniform int lightNum;


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
    fragmentColor = vec4(0.0,0.0,0.0,diffuseColor.a);
	vec4 normal = normalize(worldNormal);
    for(int p = 0; p < lightNum; p++) {
        vec4 lightPositionOrDir = lights[p].position;
        vec3 powerDensity = lights[p].powerDensity;
        vec4 viewDir = normalize(camera.position - worldPosition);
		
        fragmentColor.rgb += shade(worldPosition, normal, viewDir, lightPositionOrDir, powerDensity, diffuseColor);
    }
}