#version 330 core
out vec4 FragColor;

uniform vec3 LSCol;

void main()
{
    FragColor = vec4(LSCol, 1.0);
}


