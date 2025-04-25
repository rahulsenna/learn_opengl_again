#version 410 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;

out vec3 vertexColor;

uniform float greenUniform; 

void main()
{
    vec3 pos = vec3(aPos.x*greenUniform, -aPos.y, aPos.z);
    gl_Position = vec4(pos,1);
    vertexColor = aPos;
}