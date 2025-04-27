#version 410 core

in vec3 vertex_color;
in vec2 text_coord;
out vec4 FragColor;

uniform sampler2D plane_texture;

void main()
{
    // FragColor = vec4(vertex_color, 1.0);
    // FragColor = vec4(1,1,1, 1.0);
    FragColor = texture(plane_texture, text_coord);
}