#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D texture_diffuse1;
uniform vec2 uTexelSize;
uniform vec2 uInvViewport;
uniform vec2 rectMin;
uniform vec2 rectMax;

void main() {
    int blurIntensity = 3; 
    int darkness = 2;
    vec3 color = vec3(0.0);


    vec2 fragUV = gl_FragCoord.xy * uInvViewport;

    bool inside = fragUV.x >= rectMin.x && fragUV.x <= rectMax.x &&  
                  fragUV.y >= rectMin.y && fragUV.y <= rectMax.y; 

    color = texture(texture_diffuse1, TexCoords).rgb;
    if(!inside) {
        color *= 0.15f;  
    }

    FragColor = vec4(color, 1.0);
}
