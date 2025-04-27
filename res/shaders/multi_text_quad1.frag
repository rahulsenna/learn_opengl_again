#version 410 core

in vec3 vertex_color;
in vec2 text_coord;
out vec4 FragColor;

// texture samplers
uniform sampler2D texture1;
uniform sampler2D texture2;

void main()
{
    // FragColor = vec4(vertex_color, 1.0);
    // FragColor = vec4(1,1,1, 1.0);
    FragColor = texture(texture1, text_coord);
    // FragColor = mix(texture(texture1, TexCoord), texture(texture2, TexCoord), 0.2);

}