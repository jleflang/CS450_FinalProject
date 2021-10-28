#version 460
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

layout (location = 0) out vec2 vTexCoords;
layout (location = 1) out vec3 vPos;
layout (location = 2) out vec3 vNormal;

uniform mat4 uProj;
uniform mat4 uView;
uniform mat4 uModel;

void main()
{
    vTexCoords = aTexCoords;
    vPos = vec3(uModel * vec4(aPos, 1.0));
    vNormal = mat3(uModel) * aNormal;   

    gl_Position =  uProj * uView * vec4(vPos, 1.0);
}