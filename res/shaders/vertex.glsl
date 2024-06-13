#version 450 core

layout(location = 0) in vec4 vColor;
layout(location = 1) in vec3 vPosition;
layout(location = 2) in vec2 vTexCoord;
layout(location = 3) in float vTexIndex;

layout(location = 0) out vec4 fColor;
layout(location = 1) out vec2 fTexCoord;
layout(location = 2) out flat float fTexIndex;

void main()
{
    gl_Position = vec4(vPosition, 1.0);
    // fColor = vColor;
    fColor = vec4(1.0);
    fTexCoord = vTexCoord;
    fTexIndex = vTexIndex;
}
