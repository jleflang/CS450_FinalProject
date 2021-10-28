#version 460
layout (location = 0) in vec3 aPos;

uniform mat4 uProj;
uniform mat4 uView;

layout (location = 0) out vec3 vPos;

void 
main()
{
    vPos = aPos;

	mat4 rotView = mat4(mat3(uView));
	vec4 clipPos = uProj * rotView * vec4(vPos, 1.0);

	gl_Position = clipPos.xyww;
}