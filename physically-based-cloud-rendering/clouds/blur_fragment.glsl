#version 460 core
out vec4 FragColor;
  
in vec2 uvs;

uniform sampler2D full_screen;

uniform bool horizontal;
uniform float weight[2] = float[] (0.44198,
0.27901);

void main()
{             
    vec2 tex_offset = 1.0 / textureSize(full_screen, 0);
    vec3 result = texture(full_screen, uvs).rgb * weight[0];
    if(horizontal)
    {
        for(int i = 1; i < 2; ++i)
        {
            result += texture(full_screen, uvs + vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
            result += texture(full_screen, uvs - vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
        }
    }
    else
    {
        for(int i = 1; i < 2; ++i)
        {
            result += texture(full_screen, uvs + vec2(0.0, tex_offset.y * i)).rgb * weight[i];
            result += texture(full_screen, uvs - vec2(0.0, tex_offset.y * i)).rgb * weight[i];
        }
    }
    FragColor = vec4(result, 1.0);
}
