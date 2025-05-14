#version 460 core
precision highp float;

in vec2 texCoord;

uniform sampler2D depthTexture;
uniform float blurScale;
uniform float blurDepthFalloff;
uniform float smoothingKernelSize;

uniform struct CameraStruct {
    mat4 viewMatrix;
	mat4 projectionMatrix;
	mat4 projectionMatrixInverse;
    vec4 position;
} camera;

#define PARAM_NUM_X 10
#define PARAM_NUM_Y 40

struct FragmentParamX {
	int paramNum;
	int paramIndexes[PARAM_NUM_X];
};

struct FragmentParamY {
	int paramNum;
	int paramIndexes[PARAM_NUM_Y];
};

layout(std430, binding = 20) restrict readonly buffer pixelParamsInSSBO {
	FragmentParamX pixelParamsIn[];
};

layout(std430, binding = 30) restrict writeonly buffer pixelParamsOutSSBO {
	FragmentParamY pixelParamsOut[];
};

uniform bool calculateParams;


vec3 uvToEye(vec2 texCoord, float depth) {
	vec4 ndc = vec4(texCoord * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
	vec4 eyeSpacePos = camera.projectionMatrixInverse * ndc;
	return eyeSpacePos.xyz / eyeSpacePos.w;
}

vec3 getEyePos(sampler2D depthTexture, vec2 texCoord) {
	return uvToEye(texCoord, texture(depthTexture, texCoord).x);
}

void main() {
	const vec3 eyeSpacePos = getEyePos(depthTexture, texCoord);
	const vec2 textSize = textureSize(depthTexture, 0);
	float depth = 0.0;
	float weightSum = 0.0;
	const float curDepth = texture(depthTexture, texCoord).x;
	
	FragmentParamY param;
	param.paramNum = 0;

	if(curDepth == 1.0){
		if(calculateParams){
			pixelParamsOut[int(gl_FragCoord.y) * int(textSize.x) + int(gl_FragCoord.x)].paramNum = 0;
		}
		discard;
		return;
	}

	const vec4 offsetOnScreen = camera.projectionMatrix * vec4(eyeSpacePos + vec3(0, smoothingKernelSize * 0.5, 0), 1.0);
	const float offsetOnScreenSize = offsetOnScreen.y / offsetOnScreen.w * 0.5 + 0.5 - texCoord.y;

	const float texelSize = 1.0 / textSize.y;
	int filterRadius = int(offsetOnScreenSize * textSize.y);
	if(filterRadius > 51)
		filterRadius = 51;
	if(filterRadius < 1)
		filterRadius = 1;

	float blurScaleCorrected = blurScale / float(filterRadius) * 35.0;
			
	for(float x=-filterRadius; x<=filterRadius; x+=1.0) {
		vec2 coord = texCoord + vec2(0.0, texelSize) * x;
		float sampleDepth = texture(depthTexture, coord).x;
		if(sampleDepth < 1.0){
			// spatial domain
			float r = x * blurScaleCorrected;
			float w = exp(-r*r);
			// range domain
			float r2 = (sampleDepth - curDepth) * blurDepthFalloff;
			float g = exp(-r2*r2);
			depth += sampleDepth * w * g;
			weightSum += w * g;

			if(!calculateParams)
			continue;

			int pixelIndex = (int(gl_FragCoord.y) + int(x + 0.1)) * int(textSize.x) + int(gl_FragCoord.x);
			FragmentParamX paramIn = pixelParamsIn[pixelIndex];
			for(int p = 0; p < paramIn.paramNum; p++){
				bool found = false;
				for(int j = 0; j < param.paramNum; j++){
					if(paramIn.paramIndexes[p] == param.paramIndexes[j]){
						found = true;
						break;
					}
				}
				if(!found && param.paramNum < PARAM_NUM_Y){
					param.paramIndexes[param.paramNum] = paramIn.paramIndexes[p];
					param.paramNum++;
				}
			}
		}
	}
	
	if(weightSum == 0.0){
		if(calculateParams){
			pixelParamsOut[int(gl_FragCoord.y) * int(textSize.x) + int(gl_FragCoord.x)].paramNum = 0;
		}
		discard;
		return;
	}
    
	if(calculateParams){
		pixelParamsOut[int(gl_FragCoord.y) * int(textSize.x) + int(gl_FragCoord.x)] = param;
	}
    gl_FragDepth = depth / weightSum;
}
