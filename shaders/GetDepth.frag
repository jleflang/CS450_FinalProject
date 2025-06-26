#version 450
layout (location = 0) out vec4 fFragColor;

layout (location = 0) in float vDepth;

void main()
{   
    float dx = dFdx(vDepth);
	float dy = dFdy(vDepth);
	float moment2 = vDepth * vDepth + 0.25 * (dx * dx + dy * dy);
	
    fFragColor = vec4(vDepth, moment2, 0., 1.);
}