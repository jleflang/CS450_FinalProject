#version 450
layout (location = 0) out vec4 FragColor;
layout (location = 0) in vec3 vPos;

layout (binding = 0) uniform samplerCube uenvMap;

uniform float uExpose;

const float A = 6.2;

vec3 
tonemapFilmic(vec3 x) 
{
  vec3 X = max(vec3(0.0), x - 0.004);
  vec3 AX = A * X;
  AX *= X;
  vec3 result = fma(X, vec3(0.5), AX);
  vec3 X2 = fma(X, vec3(1.7), AX);
  result /= (X2 + 0.06);
  return pow(result, vec3(2.2));
}

void 
main()
{		
    vec3 envColor = texture(uenvMap, vPos).rgb;
    
    // HDR tonemap and gamma correct
	envColor = tonemapFilmic(envColor * uExpose);
    //envColor = envColor / (envColor + vec3(1.0));
    //envColor = pow(envColor, vec3(0.4545454545));
    
    FragColor = vec4(envColor, 1.0);
}