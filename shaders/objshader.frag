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
const float heightScale = 0.01;

vec3 ContactRefineParallax(vec2 uv)
{
    const int refinementSteps = 64;
    const int maxSteps = 32;
    
    vec3 viewDir = normalize(vpTBN * uCamPos - vpTBN * vPosVS.xyz);
    
    // Steep Parallax Mapping for approximate intersection
    vec2 currentTexCoords = uv;
    float layerD = 1. / float(maxSteps);
    float currentLayerD = 0.0;
    float currentHeight = texture(heighttex, currentTexCoords).r;
    vec2 deltaUV = viewDir.xy * (heightScale * currentHeight);

    for (int i = 0; i < maxSteps; i++) {
        currentTexCoords -= deltaUV;
        currentLayerD += layerD;
        currentHeight = texture(heighttex, currentTexCoords).r;

        if (currentHeight < currentLayerD) { // Check if ray is below surface
            break;
        }
    }

    // Contact Refinement (e.g., Binary Search)
    
    vec2 low = currentTexCoords - layerD;
    vec2 high = currentTexCoords;
    float refineD = 1. / float(refinementSteps);
    float midHeight = 0.0;

    for (int i = 0; i < refinementSteps; i++) {
        vec2 mid = (low + high) * 0.5;
        midHeight = texture(heighttex, mid).r;

        if (midHeight < refineD) {
            high = mid;
        } else {
            low = mid;
        }
    }

    return vec3(uv, midHeight);
}

vec3 getNormalFromMap(vec2 uv)
{
    vec3 tanNorm = texture(normtex, uv).rgb;
    vec3 tangentNormal = tanNorm * 2.0 - 1.0;
    tangentNormal.g = 1. - tangentNormal.g;

    return normalize(tangentNormal);
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
    uv += -0.0;
    vec3 V = normalize(uCamPos-vPos.xyz);

    vec3 uvh = ContactRefineParallax(uv);
    uv = uvh.xy;

    vec3 N = getNormalFromMap(uv);
    vec3 R = reflect(-V, N);

    vec3 tdiffuse   = pow(texture(diffusetex, uv).rgb, vec3(uExpose));
    float tmetal    = texture(metallictex, uv).r;
    float trough    = texture(roughtex, uv).r + uvh.z;
    
    //if (uv.x < 0. || uv.x > 1. || uv.y < 0. || uv.y > 1.) discard;

    // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0 
    // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)    
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, tdiffuse, tmetal);
	
	float NdotV = dot(N, V);
    
    // ambient lighting (we now use IBL as the ambient term)
    vec3 F = fresnelSchlickRoughness(NdotV, F0, trough);

    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - tmetal;

    vec3 irradiance = texture(iemMap, N).rgb;
    vec3 diffuse    = irradiance * tdiffuse;

    // sample both the pre-filter map and the BRDF lut and combine them together as per the Split-Sum approximation to get the IBL specular part.
    const float MAX_REFLECTION_LOD = 4.0;
    vec3 prefilteredColor = textureLod(prefilMap, R,  trough * MAX_REFLECTION_LOD).rgb;
    vec2 brdf  = texture(brdfLUT, vec2(NdotV, trough)).rg;
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
        vec3 lightDir = vpTBN * L;

        // Shadows: 0.0 < shadow < 1.0, where shadow is a light allowance factor
        float dc = max(0.0, dot(-lightDir, N));
        float shadow = dc > 0.0 ? ComputeShadow(lightDir, i) : 1.0;

        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, trough);
        float G   = GeometrySmith(N, V, L, trough);
        vec3 F    = fresnelSchlick(clamp(dot(H, V), 0.0, 1.0), F0);
        float NdotL = dot(N, L);
           
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
        
        // Oren-Nayar diffuse term
        vec3 ondif = kD * orennayar(dot(L, V), NdotL, NdotV, trough, tdiffuse);

        // Cook-Torrance specular term
        vec3 ct = fma(kD, (tdiffuse / PI), specular);
        
        Lo += radiance * mix(ondif, ct, fresnelSchlickRoughness(NdotL, F0, trough)) * shadow * max(NdotL, 0.0);
    }
    
    Lo += ambient;

    // HDR tonemapping
    vec3 color = tonemapFilmic(Lo * uExpose);
    //color = color / (color + vec3(1.0));
    // gamma correct
    //color = pow(color, vec3(0.454545454545));

    FragColor = vec4(color, 1.0);
}