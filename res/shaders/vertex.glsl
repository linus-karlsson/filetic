#version 450 core

layout(location = 0) in vec4 vPosition;
layout(location = 1) in vec4 vColor;

layout(location = 1) out vec4 fColor;

void main()
{
    gl_Position = vPosition;
    fColor = vColor;
}
