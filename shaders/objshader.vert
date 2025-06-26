#version 450
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

layout (location = 1) out vec3 vPos;
layout (location = 2) out vec3 vNormal;
layout (location = 3) out vec2 vTexCoords;
layout (location = 4) out vec4 vFragPosLightSpace[4];

uniform mat4 uProj;
uniform mat4 uView;
uniform mat4 uModel;
uniform mat3 uModelMatrix;
uniform mat4 uLightSpaceMatrix[4];

void main()
{
    vTexCoords = aTexCoords;
    vPos = vec3(uModel * vec4(aPos, 1.0));
    vNormal = uModelMatrix * aNormal;
    for (int i = 0; i < 4; i++)
        vFragPosLightSpace[i] = uLightSpaceMatrix[i]  * vec4(vPos, 1.);

    gl_Position =  uProj * uView * vec4(vPos, 1.0);
}