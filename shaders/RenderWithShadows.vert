#version 450
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

uniform mat4  uLightSpaceMatrix;
uniform mat4  uModel;
uniform mat4  uView;
uniform mat4  uProj;
uniform vec3 uLight;

layout (location = 0) out vec4 vFragPosLightSpace;
layout (location = 1) out vec3 vNs;
layout (location = 2) out vec3 vLs;
layout (location = 3) out vec3 vEs;

void
main()
{

	vec4 ECposition = uView * uModel * vec4(aPos, 1.);
    vec3 tnorm = normalize( mat3(uModel) * aNormal );
	vNs = tnorm;
	vLs = uLight      - ECposition.xyz;
	vEs = vec3( 0., 0., 0. ) - ECposition.xyz;
        
	vFragPosLightSpace = uLightSpaceMatrix  * uModel * vec4(aPos, 1.);
	gl_Position        = uProj * uView      * uModel * vec4(aPos, 1.);
}