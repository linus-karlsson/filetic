#version 450 core

layout(location = 0) in vec4 fColor;
layout(location = 1) in vec2 fTexCoord;
layout(location = 2) in flat float fTexIndex;

layout(location = 0) out vec4 finalColor;

uniform sampler2D textures[50];

void main()
{
    int index = int(fTexIndex);

    const float blurAmount = 0.0026;
    const int samples = 25; 

    vec4 colorSum = vec4(0.0);
    float weightSum = 0.0;

    for (int x = -samples; x <= samples; ++x)
    {
        for (int y = -samples; y <= samples; ++y)
        {
            float weight =
                max(0.0, 1.0 - sqrt(float(x * x + y * y)) / float(samples));
            vec2 offset = vec2(float(x) * blurAmount, float(y) * blurAmount);
            colorSum += texture(textures[index], fTexCoord + offset) * weight;
            weightSum += weight;
        }
    }

    finalColor = (colorSum / weightSum) * fColor;
}

