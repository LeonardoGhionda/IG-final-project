#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D texture_diffuse1;
uniform bool hasTexture;
uniform vec3 diffuseColor;  

void main()
{
    vec4 outColor;
    if (hasTexture) {
        outColor = texture(texture_diffuse1, TexCoords);
    } else {
        outColor = vec4(diffuseColor, 1.0); 
    }

    FragColor = outColor;
}

