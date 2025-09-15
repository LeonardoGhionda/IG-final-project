#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

uniform sampler2D diffuseMap;

// luce puntiforme in centro schermo, davanti alla camera
uniform vec3 lightPos = vec3(0.0, 0.0, 10.0);
uniform vec3 lightColor = vec3(1.0, 1.0, 1.0);

void main()
{ 
    float attenuation_coeff = 0.00001;

    vec3 N = normalize(Normal);
    vec3 L = normalize(lightPos - FragPos);

    // Lambert
    float diff = max(dot(N, L), 0.0);

    // attenuazione in base alla distanza
    float distance = length(lightPos - FragPos);
     
    float attenuation = 1.0 / (1.0 + attenuation_coeff * 50 * distance + attenuation_coeff * distance * distance);

    vec3 texColor = texture(diffuseMap, TexCoords).rgb;
    vec3 ambient = 0.1f * texColor;
    vec3 result = ambient + texColor * lightColor * diff * attenuation;

    FragColor = vec4(result, 1.0);
}