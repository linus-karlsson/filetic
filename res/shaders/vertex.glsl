#version 100

attribute vec3 vPos;
attribute vec4 vColor;

uniform mat4 modelView;
uniform mat4 projection;

varying vec4 fColor;

void main()
{
    gl_Position = projection * modelView * vec4(vPos, 1.0);
    fColor = vColor;
}
