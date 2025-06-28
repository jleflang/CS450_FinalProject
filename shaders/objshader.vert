#version 450
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

layout (location = 1) out vec4 vPos;
layout (location = 2) out vec4 vPosVS;
layout (location = 3) out vec3 vNormal;
layout (location = 4) out vec2 vTexCoords;
layout (location = 5) out vec4 vFragPosLightSpace[4];
layout (location = 9) out mat3 vpTBN;
layout (location = 12) out mat3 vpTBNinv;

uniform mat4 uProj;
uniform mat4 uView;
uniform mat4 uModel;
uniform mat3 uModelMatrix;
uniform mat4 uLightSpaceMatrix[4];

void main()
{
    vTexCoords = aTexCoords;
    vec4 pos = vec4(aPos, 1.0);
    vPos = vec4(uModel * pos);
    vPosVS = vec4(uView * pos);
    vNormal = normalize(uModelMatrix * aNormal);
    vec3 T = normalize(uModelMatrix * aTangent);
    vec3 B = normalize(uModelMatrix * aBitangent);
    vpTBN = mat3(T, B, vNormal);
    vpTBNinv = transpose(vpTBN);
    for (int i = 0; i < 4; i++)
        vFragPosLightSpace[i] = uLightSpaceMatrix[i]  * vPos;

    gl_Position =  uProj * uView * vPos;
}