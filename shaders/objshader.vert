#version 330 compatibility
in vec3 aPos;
in vec3 aNormal;
in vec2 aTexCoords;

out vec2 vTexCoords;
out vec3 vPos;
out vec3 vNormal;

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