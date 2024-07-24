#version 450 core

layout(location = 0) in vec4 fColor;
layout(location = 1) in vec2 fTexCoord;
layout(location = 2) in flat float fTexIndex;

layout(location = 0) out vec4 finalColor;

uniform sampler2D textures[50];

void main()
{
    int index = int(fTexIndex);

    const float blurAmount = 0.0001;
    const int samples = 100;

    vec3 colorSum = vec3(0.0);
    float weightSum = 0.0;

    for (int i = -samples; i <= samples; ++i)
    {
        float weight = 1.0 - abs(float(i)) / float(samples + 1);
        colorSum += texture(textures[index],
                            fTexCoord + vec2(blurAmount * float(i), 0.0))
                        .rgb *
                    weight;
        weightSum += weight;
    }

    finalColor = vec4((colorSum / weightSum) * fColor.rgb, 1.0);
}

