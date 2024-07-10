#version 450 core

layout(location = 0) in vec4 vColor;
layout(location = 1) in vec3 vPosition;
layout(location = 2) in vec3 vNormal;
layout(location = 3) in vec2 vTexCoord;
layout(location = 4) in float vTexIndex;

layout(location = 0) out vec4 fColor;
layout(location = 1) out vec2 fTexCoord;
layout(location = 2) out flat float fTexIndex;

uniform mat4 proj;
uniform mat4 view;
uniform mat4 model;

void main()
{
    gl_Position = proj * view * mat4(1.0f) * vec4(vPosition, 1.0);

    vec3 light_pos = vec3(model[0]);
    float intensity = dot(vNormal, light_pos);
    vec3 final_color = vColor.rgb * intensity;
    fColor = vec4(final_color, vColor.a);

    fTexCoord = vTexCoord;
    fTexIndex = vTexIndex;
}
