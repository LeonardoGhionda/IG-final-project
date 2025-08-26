#version 330 core
out vec4 FragColor;

uniform vec3 trailColor;

void main() {
    FragColor = vec4(trailColor, 1.0);
}
