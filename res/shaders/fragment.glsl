#version 450 core

layout(location = 1) in vec4 fColor;

layout(location = 0) out vec4 finalColor;

void main()
{
   finalColor = fColor;
}
