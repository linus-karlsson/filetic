#version 450 core

layout(location = 0) in vec4 vColor;
layout(location = 1) in vec3 vPosition;

layout(location = 1) out vec4 fColor;

void main()
{
    gl_Position = vec4(vPosition, 1.0);
    //fColor = vColor;
    fColor = vec4(1.0);
}
