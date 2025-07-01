#version 450

layout (location = 0) out vec4 FragColor;
layout (location = 1) in vec4 vPos;
layout (location = 2) in vec4 vPosVS;
layout (location = 3) in vec3 vNormal;
layout (location = 4) in vec2 vTexCoords;
layout (location = 5) in vec4 vFragPosLightSpace[4];
layout (location = 9) in mat3 vpTBN;
layout (location = 12) in mat3 vpTBNinv;

// material parameters
uniform float ao;
uniform float uExpose;
uniform float uTexScale;

// Environment textures
layout (binding = 1) uniform samplerCube iemMap;
layout (binding = 2) uniform samplerCube prefilMap;
layout (binding = 3) uniform sampler2D brdfLUT;
layout (binding = 4) uniform sampler2DArray shadowMap;

// PBR textures
layout (binding = 5) uniform sampler2D diffusetex;
layout (binding = 6) uniform sampler2D roughtex;
layout (binding = 7) uniform sampler2D metallictex;
layout (binding = 8) uniform sampler2D normtex;
layout (binding = 9) uniform sampler2D heighttex;

// Camera View
uniform vec3 uCamPos;

// lights
uniform vec3 lightPositions[4];
uniform vec3 lightColors[4];

const float PI = 3.14159265359;
const float A = 6.2;
const float heightScale = 0.05;

// Adapted from https://github.com/panthuncia/webgl_test/blob/main/index.html
vec3 ContactRefineParallax(vec2 uv)
{
    
    vec3 tCam = vpTBNinv * uCamPos;
    vec3 tFrag = vpTBNinv * vPos.xyz;
    vec3 viewDir = normalize(tCam - tFrag);

    float maxHeight = heightScale;
    float minHeight = maxHeight*1.0;

    int mSteps = 16;

    float viewCorr = (-viewDir.z) + 2.0;
    float stepSz = 1.0 / (float(mSteps) + 1.0);
    vec2 sOffset = viewDir.xy * vec2(maxHeight, maxHeight) * stepSz;

    vec2 lastOff = fract(fma(viewDir.xy, vec2(minHeight, minHeight), uv) + 1.0);
    float lastRayD = 1.0;
    float lastH = 1.0;

    vec2 p1;
    vec2 p2;
    bool refine = false;

    while (mSteps > 0) {
        // Advance ray in direction of TS view direction
        vec2 candidateOffset = fract((lastOff-sOffset) + 1.0);
        float currentRayDepth = lastRayD - stepSz;

        // Sample height map at this offset
        float currentHeight = texture(heighttex, candidateOffset).r;
        currentHeight = viewCorr * currentHeight;
        // Test our candidate depth
        if (currentHeight > currentRayDepth)
        {
            p1 = vec2(currentRayDepth, currentHeight);
            p2 = vec2(lastRayD, lastH);
            // Break if this is the contact refinement pass
            if (refine) {
                lastH = currentHeight;
                break;
            // Else, continue raycasting with squared precision
            } else {
                refine = true;
                lastRayD = p2.x;
                stepSz /= float(mSteps);
                sOffset /= float(mSteps);
                continue;
            }
        }
        lastOff = candidateOffset;
        lastRayD = currentRayDepth;
        lastH = currentHeight;
        mSteps -= 1;
    }

    // Interpolate between final two points
    float diff1 = p1.x - p1.y;
    float diff2 = p2.x - p2.y;
    float denominator = diff2 - diff1;

    float parallaxAmount = 0.0;
    if (denominator != 0.00) {
        parallaxAmount = (p1.x * diff2 - p2.x * diff1) / denominator;
    }

    float offset = fma((1.0 - parallaxAmount), -maxHeight, minHeight);

    return vec3(viewDir.xy * offset + uv, lastH);
}

vec3 getNormalFromMap(vec2 uv)
{
    vec3 tanNorm = texture(normtex, uv).rgb;
    vec3 tangentNormal = normalize(tanNorm * 2.0 - 1.0);
    //tangentNormal.g = 1. - tangentNormal.g;

    return normalize(vpTBN * tangentNormal);
}
// ----------------------------------------------------------------------------
// Piecewise linear interpolation
float linstep(const float low, const float high, const float value) {
    return clamp((value - low) / (high - low), 0.0, 1.0);
}

// ----------------------------------------------------------------------------
// Variance shadow mapping
float ComputeShadow(const vec3 uvh, const int lightlayer) {
    //vec4 fragPosLightSpace = vFragPosLightSpace[lightlayer];

    // Perspective divide
    //vec2 screenCoords = fragPosLightSpace.xy / fragPosLightSpace.w;
    //screenCoords = screenCoords * 0.5 + 0.5; // [0, 1]
	vec2 screenCoords = uvh.xy / uvh.z;
    screenCoords = screenCoords * 0.5 + 0.5; // [0, 1]

    const float distance = uvh.z; // Use raw distance instead of linear junk	
    vec2 moments = texture(shadowMap, vec3(screenCoords, lightlayer)).rg;
	if (distance <= moments.x)
		return 1.0;

    float p = step(distance, moments.x);
    float variance = max(moments.y - (moments.x * moments.x), 0.00002);
    float d = distance - moments.x;
    float pMax = smoothstep(0.2, 1.0, variance / fma(d, d, variance)); // Solve light bleeding

   return min(max(p, pMax), 1.0);
}

vec3 tonemapFilmic(vec3 x)
{
    vec3 X = max(vec3(0.0), x - 0.004);
    vec3 AX = A * X;
    AX *= X;
    vec3 result = fma(X, vec3(0.5), AX);
    vec3 X2 = fma(X, vec3(1.7), AX);
    result /= (X2 + 0.06);
    return pow(result, vec3(2.2));
}

// Taken from https://github.com/glslify/glsl-diffuse-oren-nayar/blob/master/index.glsl
vec3 orennayar(float LdotV, float NdotL, float NdotV, float roughness, vec3 albedo)
{
	float s = LdotV - NdotL * NdotV;
	float t = mix(1.0, max(NdotL, NdotV), step(0.0, s));

	float sigma2 = roughness*roughness;
	float A = 1.0 + sigma2 * (albedo.r / (sigma2 + 0.13) + 0.5 / (sigma2 + 0.33));
	A /= 3.;
	float Ag = 1.0 + sigma2 * (albedo.g / (sigma2 + 0.13) + 0.5 / (sigma2 + 0.33));
	A += Ag / 3.;
	float Ab = 1.0 + sigma2 * (albedo.b / (sigma2 + 0.13) + 0.5 / (sigma2 + 0.33));
	A += Ab / 3.;
	float B = 0.45 * sigma2 / (sigma2 + 0.09);

	return albedo * max(0.0, NdotL) * (A + B * s / t) / PI;
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

    float nom   = 1;
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
    vec2 uv = vTexCoords;
    uv *= uTexScale;

    vec3 uvh = ContactRefineParallax(uv);
    uv = uvh.xy;

    vec3 V = normalize(uCamPos-vPos.xyz);
    vec3 N = getNormalFromMap(uv);
    vec3 R = reflect(V, N);

    vec3 tdiffuse	= pow(texture(diffusetex, uv).rgb, vec3(uExpose));
    float tmetal	= texture(metallictex, uv).r;
    float trough	= texture(roughtex, uv).r;

    // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0 
    // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)    
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, tdiffuse, tmetal);
	
	// ambient lighting (we now use IBL as the ambient term)
    vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, trough);

    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - tmetal;

    vec3 irradiance = texture(iemMap, N).rgb;
    vec3 diffuse    = irradiance * tdiffuse;

    // sample both the pre-filter map and the BRDF lut and combine them together as per the Split-Sum approximation to get the IBL specular part.
    const float MAX_REFLECTION_LOD = 4.0;
    vec3 prefilteredColor = textureLod(prefilMap, R,  trough * MAX_REFLECTION_LOD).rgb;
    vec2 brdf  = texture(brdfLUT, vec2(max(dot(N, V), 0.0), trough)).rg;
    vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

    vec3 ambient = fma(kD, diffuse, specular) * ao;

    // reflectance equation
    vec3 Lo = vec3(0.0);
    for(int i = 0; i < 4; ++i)
    {
        // calculate per-light radiance
        vec3 L = normalize(lightPositions[i] - vPos.xyz);
        vec3 H = normalize(V + L);
        float distance = length(lightPositions[i] - vPos.xyz);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = lightColors[i] * attenuation;
        vec3 lightDir = vpTBNinv * L;

        // Shadows: 0.0 < shadow < 1.0, where shadow is a light allowance factor
        float dc = max(0.0, dot(-lightDir, N));
        float shadow = dc > 0.0 ? ComputeShadow(-lightDir, i) : 1.0;

        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, trough);
        float G   = GeometrySmith(N, V, L, trough);
        vec3 F    = fresnelSchlick(clamp(dot(H, V), 0.0, 1.0), F0);
		float NdotL = dot(N, L);
		float NdotV = dot(N, V);
           
        vec3 numerator    = NDF * G * F;
        float denominator = 4;
        vec3 specular = numerator / denominator;
        
        // kS is equal to Fresnel
        vec3 kS = F;
        // for energy conservation, the diffuse and specular light can't
        // be above 1.0 (unless the surface emits light); to preserve this
        // relationship the diffuse component (kD) should equal 1.0 - kS.
        vec3 kD = vec3(1.0) - kS;
        // multiply kD by the inverse metalness such that only non-metals 
        // have diffuse lighting, or a linear blend if partly metal (pure metals
        // have no diffuse light).
        kD *= 1.0 - tmetal;
		
		// Use Oren-Nayar for the diffuse term
		vec3 ondif = orennayar(dot(L, V), NdotL, NdotV, trough, tdiffuse);
		
		// scale light by NdotL
        NdotL = max(dot(N, L), 0.0);

        // add to outgoing radiance Lo
        // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
        Lo += fma(kD, ondif, specular) * radiance * shadow * NdotL;
    }
	
	Lo += ambient;

    // HDR tonemapping
    vec3 color = tonemapFilmic(Lo * uExpose);
    //color = color / (color + vec3(1.0));
    // gamma correct
    //color = pow(color, vec3(0.454545454545));

    FragColor = vec4(color, 1.0);
}