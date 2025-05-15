#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D texture_diffuse1;
uniform bool hasTexture;
uniform vec3 colorCoeff;

vec4 RED = vec4(1.0,0.0,0.0,1.0);
vec4 GREEN = vec4(0.0,1.0,0.0,1.0);
vec4 BLUE = vec4(0.0,0.0,1.0,1.0);

void main()
{
    vec4 outColor;
    if (hasTexture) { 
        outColor = texture(texture_diffuse1, TexCoords); 
    }
    else { 
        outColor = colorCoeff[0] * RED + colorCoeff[1] * GREEN + colorCoeff[2] * BLUE;
    }
    FragColor = outColor;
}
