#version 330 core
out vec4 FragColor;

uniform vec4 overlayColor;

vec4 RED = vec4(1.0,0.0,0.0,1.0);
vec4 GREEN = vec4(0.0,1.0,0.0,1.0);
vec4 BLUE = vec4(0.0,0.0,1.0,1.0);

void main() {
    FragColor = overlayColor;
}
