#version 450 core

layout(location = 0) in vec4 vColor;
layout(location = 1) in vec2 vPosition;
layout(location = 2) in vec2 vTexCoord;
layout(location = 3) in float vTexIndex;

layout(location = 0) out vec4 fColor;
layout(location = 1) out vec2 fTexCoord;
layout(location = 2) out flat float fTexIndex;

uniform mat4 proj;
uniform mat4 view;
uniform mat4 model;

void main()
{
    gl_Position = proj * view * model * vec4(vPosition, 0.0, 1.0);
    fColor = vColor;
    fTexCoord = vTexCoord;
    fTexIndex = vTexIndex;
}
