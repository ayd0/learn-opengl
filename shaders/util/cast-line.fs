#version 330 core
out vec4 FragColor;

uniform bool alt;

void main()
{             
    if (alt)
        FragColor = vec4(0.0f, 1.0f, 0.0f, 1.0f);
    else
        FragColor = vec4(1.0f, 0.0f, 0.0f, 1.0f);
}
