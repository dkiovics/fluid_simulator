#version 460 core
precision highp float;

out vec4 fragmentColor;

in vec2 texCoord;

uniform sampler2D normalAndDepthTexture;
uniform sampler2D thicknessTexture;
uniform float transparency;
uniform int transparencyEnabled;
uniform int noiseEnabled;
uniform float noiseScale;
uniform float noiseStrength;
uniform float noiseOffset;
uniform int lightNum;

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

vec3 uvToEye(vec2 texCoord, float depth) {
	vec4 ndc = vec4(texCoord * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
	vec4 eyeSpacePos = camera.projectionMatrixInverse * ndc;
	return eyeSpacePos.xyz / eyeSpacePos.w;
}

vec3 uvToWorldPos(vec2 texCoord, float depth) {
	return (camera.viewMatrixInverse * vec4(uvToEye(texCoord, depth), 1.0)).xyz;
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

float cnoise(vec3 P);

vec3 getNoiseOffsetWorldSpacePos(vec2 texCoord) {
	vec4 normalAndDepth = texture(normalAndDepthTexture, texCoord);
	vec3 worldSpacePos = uvToWorldPos(texCoord, normalAndDepth.a);
	vec3 pos = worldSpacePos * noiseScale;
	float noise = cnoise(vec3(pos.x + noiseOffset, pos.y, pos.z)) + cnoise(vec3(-pos.x + noiseOffset, pos.y + noiseOffset, pos.z));
	return worldSpacePos + normalAndDepth.xyz * noise;
}

vec3 getNormalWithNoise() {
	vec3 posEye = getNoiseOffsetWorldSpacePos(texCoord);
	vec2 texelSize = vec2(1.0, 1.0) / textureSize(normalAndDepthTexture, 0);
	
	vec3 ddx = getNoiseOffsetWorldSpacePos(texCoord + vec2(texelSize.x, 0)) - posEye;
	vec3 ddx2 = posEye - getNoiseOffsetWorldSpacePos(texCoord + vec2(-texelSize.x, 0));
	if (abs(ddx.z) > abs(ddx2.z)) {
		ddx = ddx2;
	}
	vec3 ddy = getNoiseOffsetWorldSpacePos(texCoord + vec2(0, texelSize.y)) - posEye;
	vec3 ddy2 = posEye - getNoiseOffsetWorldSpacePos(texCoord + vec2(0, -texelSize.y));
	if (abs(ddy2.z) < abs(ddy.z)) {
		ddy = ddy2;
	}
	return normalize(cross(ddx, ddy));
}

void main(){
	vec4 normalAndDepth = texture(normalAndDepthTexture, texCoord);
	if (normalAndDepth.a == 1.0f) {
		discard;
		return;
	}
	
	gl_FragDepth = normalAndDepth.a;
	
	vec3 posEye = uvToEye(texCoord, normalAndDepth.a);
	vec4 normal = vec4(normalAndDepth.xyz, 0);
	vec4 worldPosition = camera.viewMatrixInverse * vec4(posEye, 1.0);
	
	if(noiseEnabled == 1) {
		normal = normalize(normal * (1.0 - noiseStrength) + vec4(getNormalWithNoise(), 0) * noiseStrength);
	}
	
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
	
	//fragmentColor = vec4(texture(noiseTexture, texCoord).r, 0, 0, 0);
}








vec3 mod289(vec3 x)
{
  return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec4 mod289(vec4 x)
{
  return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec4 permute(vec4 x)
{
  return mod289(((x*34.0)+10.0)*x);
}

vec4 taylorInvSqrt(vec4 r)
{
  return 1.79284291400159 - 0.85373472095314 * r;
}

vec3 fade(vec3 t) {
  return t*t*t*(t*(t*6.0-15.0)+10.0);
}

// Classic Perlin noise
float cnoise(vec3 P)
{
  vec3 Pi0 = floor(P); // Integer part for indexing
  vec3 Pi1 = Pi0 + vec3(1.0); // Integer part + 1
  Pi0 = mod289(Pi0);
  Pi1 = mod289(Pi1);
  vec3 Pf0 = fract(P); // Fractional part for interpolation
  vec3 Pf1 = Pf0 - vec3(1.0); // Fractional part - 1.0
  vec4 ix = vec4(Pi0.x, Pi1.x, Pi0.x, Pi1.x);
  vec4 iy = vec4(Pi0.yy, Pi1.yy);
  vec4 iz0 = Pi0.zzzz;
  vec4 iz1 = Pi1.zzzz;

  vec4 ixy = permute(permute(ix) + iy);
  vec4 ixy0 = permute(ixy + iz0);
  vec4 ixy1 = permute(ixy + iz1);

  vec4 gx0 = ixy0 * (1.0 / 7.0);
  vec4 gy0 = fract(floor(gx0) * (1.0 / 7.0)) - 0.5;
  gx0 = fract(gx0);
  vec4 gz0 = vec4(0.5) - abs(gx0) - abs(gy0);
  vec4 sz0 = step(gz0, vec4(0.0));
  gx0 -= sz0 * (step(0.0, gx0) - 0.5);
  gy0 -= sz0 * (step(0.0, gy0) - 0.5);

  vec4 gx1 = ixy1 * (1.0 / 7.0);
  vec4 gy1 = fract(floor(gx1) * (1.0 / 7.0)) - 0.5;
  gx1 = fract(gx1);
  vec4 gz1 = vec4(0.5) - abs(gx1) - abs(gy1);
  vec4 sz1 = step(gz1, vec4(0.0));
  gx1 -= sz1 * (step(0.0, gx1) - 0.5);
  gy1 -= sz1 * (step(0.0, gy1) - 0.5);

  vec3 g000 = vec3(gx0.x,gy0.x,gz0.x);
  vec3 g100 = vec3(gx0.y,gy0.y,gz0.y);
  vec3 g010 = vec3(gx0.z,gy0.z,gz0.z);
  vec3 g110 = vec3(gx0.w,gy0.w,gz0.w);
  vec3 g001 = vec3(gx1.x,gy1.x,gz1.x);
  vec3 g101 = vec3(gx1.y,gy1.y,gz1.y);
  vec3 g011 = vec3(gx1.z,gy1.z,gz1.z);
  vec3 g111 = vec3(gx1.w,gy1.w,gz1.w);

  vec4 norm0 = taylorInvSqrt(vec4(dot(g000, g000), dot(g010, g010), dot(g100, g100), dot(g110, g110)));
  g000 *= norm0.x;
  g010 *= norm0.y;
  g100 *= norm0.z;
  g110 *= norm0.w;
  vec4 norm1 = taylorInvSqrt(vec4(dot(g001, g001), dot(g011, g011), dot(g101, g101), dot(g111, g111)));
  g001 *= norm1.x;
  g011 *= norm1.y;
  g101 *= norm1.z;
  g111 *= norm1.w;

  float n000 = dot(g000, Pf0);
  float n100 = dot(g100, vec3(Pf1.x, Pf0.yz));
  float n010 = dot(g010, vec3(Pf0.x, Pf1.y, Pf0.z));
  float n110 = dot(g110, vec3(Pf1.xy, Pf0.z));
  float n001 = dot(g001, vec3(Pf0.xy, Pf1.z));
  float n101 = dot(g101, vec3(Pf1.x, Pf0.y, Pf1.z));
  float n011 = dot(g011, vec3(Pf0.x, Pf1.yz));
  float n111 = dot(g111, Pf1);

  vec3 fade_xyz = fade(Pf0);
  vec4 n_z = mix(vec4(n000, n100, n010, n110), vec4(n001, n101, n011, n111), fade_xyz.z);
  vec2 n_yz = mix(n_z.xy, n_z.zw, fade_xyz.y);
  float n_xyz = mix(n_yz.x, n_yz.y, fade_xyz.x); 
  return 2.2 * n_xyz;
}