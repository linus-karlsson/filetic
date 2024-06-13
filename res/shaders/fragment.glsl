#version 450 core

layout(location = 0) in vec4 fColor;
layout(location = 1) in vec2 fTexCoord;
layout(location = 2) in flat float fTexIndex;

layout(location = 0) out vec4 finalColor;

void main()
{
   finalColor = fColor;
}
