#version 330 compatibility
in vec3 aPos;

uniform mat4 uProj;
uniform mat4 uView;

out vec3 vPos;

void
main()
{
    vPos = aPos;

    gl_Position = uProj * uView * vec4(vPos, 1.0);
}