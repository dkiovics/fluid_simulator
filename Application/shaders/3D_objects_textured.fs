#version 460 core
precision highp float;

out vec4 fragmentColor;
in vec2 textureCoords;
in vec3 worldPosition;
in vec3 worldNormal;
in vec4 diffuseColor;

uniform struct {
    sampler2D colorTexture;
	float textureScale;
    vec3 specularColor;
    float shininess;
} material;

uniform struct{
    mat4 viewMatrix;
	mat4 projectionMatrix;
    vec3 position;
} camera;

uniform struct {
  vec4 position;		//if the w component of the position is 0 than the light is directional, otherwise its a point light
  vec3 powerDensity;
} lights[100];

uniform int lightNum;


vec3 shade(vec3 worldPos, vec3 normal, vec3 viewDir, vec4 lightPos, vec3 powerDensity, vec3 materialColor) {
	vec3 frag2light = lightPos.xyz - worldPos * lightPos.w;
    vec3 lightDir = normalize(frag2light);
    if(dot(normal, viewDir) < 0.0)
        normal = -normal;
    float lightCos = dot(lightDir, normal);
    if(lightCos < 0.0)
        return vec3(0.0, 0.0, 0.0);
    float d2 = length(frag2light);
    d2 = d2*d2;
    vec3 halfWay = normalize(viewDir + lightDir);
    float shine = dot(halfWay, normal);
    if(shine < 0.0)
        shine = 0.0;
    return powerDensity * (materialColor * lightCos + material.specularColor * pow(shine, material.shininess)) / d2;
}


void main(void) {
    fragmentColor = vec4(0.0,0.0,0.0,diffuseColor.a);
	vec3 normal = normalize(worldNormal);
	vec3 color = diffuseColor.rgb * texture(material.colorTexture, textureCoords * material.textureScale).rgb;
	vec3 viewDir = normalize(camera.position - worldPosition);
	
    for(int p = 0; p < lightNum; p++) {
        vec4 lightPositionOrDir = lights[p].position;
        vec3 powerDensity = lights[p].powerDensity;
		
        fragmentColor.rgb += shade(worldPosition, normal, viewDir, lightPositionOrDir, powerDensity, color);
    }
}
