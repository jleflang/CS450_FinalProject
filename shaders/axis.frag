#version 460

layout (location = 0) out vec4 FragColor;
layout (location = 0) in vec2 vTexCoords;
layout (location = 1) in vec3 vPos;
layout (location = 2) in vec3 vNormal;
layout (location = 3) in vec4 vFragPosLightSpace;

// material parameters
uniform vec3 albedo;
uniform float metallic;
uniform float roughness;
uniform float ao;

uniform float uExpose;

// Environment textures
layout (binding = 0) uniform samplerCube iemMap;
layout (binding = 1) uniform samplerCube prefilMap;
layout (binding = 2) uniform sampler2D brdfLUT;
layout (binding = 3) uniform sampler2D shadowMap;

uniform vec3 uCamPos;

// lights
uniform vec3 lightPositions[4];
uniform vec3 lightColors[4];

const float PI = 3.14159265359;
const float A = 6.2;

// ----------------------------------------------------------------------------
// Piecewise linear interpolation
float linstep(const float low, const float high, const float value) {
    return clamp((value - low) / (high - low), 0.0, 1.0);
}

// ----------------------------------------------------------------------------
// Variance shadow mapping
float ComputeShadow(const vec4 fragPosLightSpace) {
    // Perspective divide
    vec2 screenCoords = fragPosLightSpace.xy / fragPosLightSpace.w;
    screenCoords = screenCoords * 0.5 + 0.5; // [0, 1]

    const float distance = fragPosLightSpace.z; // Use raw distance instead of linear junk
    const vec2 moments = texture2D(shadowMap, screenCoords.xy).rg;

    const float p = step(distance, moments.x);
    const float variance = max(moments.y - (moments.x * moments.x), 0.00002);
    const float d = distance - moments.x;
    const float pMax = linstep(0.2, 1.0, variance / fma(d, d, variance)); // Solve light bleeding

   return min(max(p, pMax), 1.0);
}

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
// ----------------------------------------------------------------------------
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) * 0.125;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}
// ----------------------------------------------------------------------------
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - min(cosTheta, 1.0), 0.0, 1.0), 5.0);
}
// ----------------------------------------------------------------------------
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - min(cosTheta, 1.0), 0.0, 1.0), 5.0);
}   
// ----------------------------------------------------------------------------
void main()
{		
    vec3 N = vNormal;
    vec3 V = normalize(uCamPos-vPos);
    vec3 R = reflect(V, -N); 
	
	const float shadow = ComputeShadow(vFragPosLightSpace);

    // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0 
    // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)    
    vec3 F0 = vec3(0.04);

	F0 = mix(F0, albedo, metallic);

    // reflectance equation
    vec3 Lo = vec3(0.0);
    for(int i = 0; i < 4; ++i) 
    {
        // calculate per-light radiance
        vec3 L = normalize(lightPositions[i] - vPos);
        vec3 H = normalize(V + L);
        float distance = length(lightPositions[i] - vPos);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = lightColors[i] * attenuation;

        // Cook-Torrance BRDF
		float NDF = DistributionGGX(N, H, roughness);   
		float G   = GeometrySmith(N, V, L, roughness);
        vec3 F    = fresnelSchlick(clamp(dot(H, V), 0.0, 1.0), F0);
           
        vec3 numerator    = NDF * G * F; 
        float denominator = 4 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0); 
        vec3 specular = numerator / max(denominator, 0.001);
        
        // kS is equal to Fresnel
        vec3 kS = F;
        // for energy conservation, the diffuse and specular light can't
        // be above 1.0 (unless the surface emits light); to preserve this
        // relationship the diffuse component (kD) should equal 1.0 - kS.
        vec3 kD = vec3(1.0) - kS;
        // multiply kD by the inverse metalness such that only non-metals 
        // have diffuse lighting, or a linear blend if partly metal (pure metals
        // have no diffuse light).
		kD *= 1.0 - metallic;
		
        // scale light by NdotL
        float NdotL = max(dot(N, L), 0.0);    

        // add to outgoing radiance Lo
		// note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
		Lo += (kD * albedo / PI + specular) * radiance * shadow * NdotL;
        
    }

	// ambient lighting (we now use IBL as the ambient term)
	vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
	
    
    vec3 kS = F;
    vec3 kD = 1.0 - kS;
	kD *= 1.0 - metallic;
    
    vec3 irradiance = texture(iemMap, N).rgb;
	vec3 diffuse      = irradiance * albedo;
    
    // sample both the pre-filter map and the BRDF lut and combine them together as per the Split-Sum approximation to get the IBL specular part.
    const float MAX_REFLECTION_LOD = 4.0;
	vec3 prefilteredColor = textureLod(prefilMap, R,  roughness * MAX_REFLECTION_LOD).rgb;    
	vec2 brdf  = texture(brdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;  
    vec3 specular = prefilteredColor * (F0 * brdf.x + brdf.y);	
    
    vec3 ambient = kD * diffuse + specular * ao;
    
    vec3 color = ambient + Lo;

    // HDR tonemapping
	color = tonemapFilmic(color * uExpose);	
    //color = color / (color + vec3(1.0));
    // gamma correct
    //color = pow(color, vec3(0.454545454545));
    
    FragColor = vec4(color, 1.0);
}