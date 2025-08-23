#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D texture_diffuse1;
uniform bool hasTexture;
uniform vec3 diffuseColor;  
uniform vec2 uTexelSize;

uniform vec2 rectMin; 
uniform vec2 rectMax; 


//shader to blur BACKGROUND if outside focusRectangle 

void main() {

    int blurIntensity = 3; 
    int darkness = 2;

    vec3 color = vec3(0.0);
    bool inside = TexCoords.x >= rectMin.x && TexCoords.x <= rectMax.x &&
                  TexCoords.y >= rectMin.y && TexCoords.y <= rectMax.y;

    if(inside) {
        color = vec3(texture(texture_diffuse1, TexCoords));
    }
    else {
        for (int x = -blurIntensity; x <= blurIntensity; x++) {
            for (int y = -blurIntensity; y <= blurIntensity; y++) {
                vec2 offset = vec2(x, y) * uTexelSize;
                color += texture(texture_diffuse1, TexCoords + offset).rgb;
            }
        }
        color /= 2*blurIntensity * 2*blurIntensity * darkness; 
    }
    
    FragColor = vec4(color, 1.0);
}