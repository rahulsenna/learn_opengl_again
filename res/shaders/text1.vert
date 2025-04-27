#version 410 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
layout(location = 2) in vec2 aUV;

out vec3 vertex_color;
out vec2 text_coord;

uniform float greenUniform; 

void main()
{
    vertex_color = aColor;
    text_coord = aUV;
    gl_Position = vec4(aPos,1);
}