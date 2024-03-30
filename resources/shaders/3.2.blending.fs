#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D texture1;

void main()
{
    vec4 greenColor = vec4(0.0, 100.0, 0.0, 1.0);
    FragColor = texture(texture1, TexCoords) * greenColor;
}