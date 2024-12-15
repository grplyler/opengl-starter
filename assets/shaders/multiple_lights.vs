#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec3 FragPos;
out vec3 Normal;
out vec3 BarycentricCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;  
    
    // Calculate barycentric coordinates
    if (gl_VertexID % 3 == 0)
        BarycentricCoords = vec3(1.0, 0.0, 0.0);
    else if (gl_VertexID % 3 == 1)
        BarycentricCoords = vec3(0.0, 1.0, 0.0);
    else
        BarycentricCoords = vec3(0.0, 0.0, 1.0);
    
    gl_Position = projection * view * vec4(FragPos, 1.0);
}