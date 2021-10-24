#version 330 compatibility
out vec4 FragColor;
in vec3 vPos;

uniform samplerCube uenvMap;

void 
main()
{		
    vec3 envColor = texture(uenvMap, vPos).rgb;
    
    // HDR tonemap and gamma correct
    envColor = envColor / (envColor + vec3(1.0));
    envColor = pow(envColor, vec3(1.0/2.2)); 
    
    FragColor = vec4(envColor, 1.0);
}