#version 410 core

in vec3 vertexColor;
out vec4 FragColor;

uniform float greenUniform; 

void main()
{
    FragColor = vec4(1-greenUniform/2.f,greenUniform,0, 1.0);
    // FragColor = vec4(vertexColor,1);
}