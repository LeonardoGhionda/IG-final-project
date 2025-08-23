#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D texture_diffuse1;
uniform bool hasTexture;
uniform vec3 diffuseColor;  
uniform vec2 uTexelSize;

uniform vec2 rectMin; 
uniform vec2 rectMax; 


void main() {
    vec3 color = vec3(0.0);
    bool inside = TexCoords.x >= rectMin.x && TexCoords.x <= rectMax.x &&
                  TexCoords.y >= rectMin.y && TexCoords.y <= rectMax.y;

    if(inside) {
        color = vec3(texture(texture_diffuse1, TexCoords));
    }
    else {
        for (int x = -2; x <= 2; x++) {
            for (int y = -2; y <= 2; y++) {
                vec2 offset = vec2(x, y) * uTexelSize;
                color += texture(texture_diffuse1, TexCoords + offset).rgb;
            }
        }
        color /= 25.0; 
    }
    
    FragColor = vec4(color, 1.0);
}

