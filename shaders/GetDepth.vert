#version 460
layout (location = 0) in vec3 aPos;

uniform mat4 uLightSpaceMatrix;
uniform mat4 uModel;

layout (location = 0) out float vDepth;

void
main()
{
    gl_Position = uLightSpaceMatrix * uModel * vec4(aPos, 1.);
	vDepth = gl_Position.z;
} 