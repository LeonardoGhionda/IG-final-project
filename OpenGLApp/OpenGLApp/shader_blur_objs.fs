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

    if(inside) {
        color = texture(texture_diffuse1, TexCoords).rgb;
    } else {
        for (int x = -blurIntensity; x <= blurIntensity; x++) {
            for (int y = -blurIntensity; y <= blurIntensity; y++) {
                vec2 offset = vec2(x, y) * uTexelSize;
                vec2 sampleUV = clamp(TexCoords + offset, 0.0, 1.0);
                color += texture(texture_diffuse1, sampleUV).rgb;
            }
        }
        color /= float(2*blurIntensity * 2*blurIntensity * darkness); 
    }

    FragColor = vec4(color, 1.0);
}
