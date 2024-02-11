#version 330 core
out vec4 FragColor;

struct Material {
    sampler2D diffuse;
    sampler2D specular;    
    float shininess;
}; 

struct DirLight {
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {
    vec3 position;

    float constant;
    float linear;
    float quadratic;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
#define NR_POINT_LIGHTS 2

struct Spotlight {
    vec3 position;
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float constant; 
    float linear;
    float quadratic;

    float cutoff;
    float outerCutoff;
};

in vec3 FragPos;  
in vec3 Normal;  
in vec2 TexCoords;
  
uniform vec3 viewPos;
uniform Material material;
uniform DirLight dirLight;
uniform PointLight pointLights[NR_POINT_LIGHTS];
uniform Spotlight spotlight;
uniform sampler2D texture_emissive1;

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);
vec3 CalcPointLights(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
vec3 CalcSpotlight(Spotlight light, vec3 normal, vec3 fragPos, vec3 viewDir);

void main()
{
    // properties
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    // directional lighting
    vec3 result = CalcDirLight(dirLight, norm, viewDir);

    // point lights
    for(int i = 0; i < NR_POINT_LIGHTS; i++)
      result += CalcPointLights(pointLights[i], norm, FragPos, viewDir);

    // spotlight
    result += CalcSpotlight(spotlight, norm, FragPos, viewDir);

    // emissive light
    vec4 emissiveColor = texture(texture_emissive1, TexCoords);
    if (emissiveColor.rgb != vec3(0.0))
    {
        result += emissiveColor.rgb;
    }

    FragColor = vec4(result, 1.0);
} 

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(-light.direction);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    //combine results
    vec3 ambient = light.ambient * vec3(texture(material.diffuse, TexCoords));
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, TexCoords));
    vec3 specular = light.specular * spec * vec3(texture(material.specular, TexCoords));

    return (ambient + diffuse + specular);
}

vec3 CalcPointLights(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    // attenuation
    float distance    = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + 
  			     light.quadratic * (distance * distance));    
    // combine results
    vec3 ambient  = light.ambient  * vec3(texture(material.diffuse, TexCoords));
    vec3 diffuse  = light.diffuse  * diff * vec3(texture(material.diffuse, TexCoords));
    vec3 specular = light.specular * spec * vec3(texture(material.specular, TexCoords));
    ambient  *= attenuation;
    diffuse  *= attenuation;
    specular *= attenuation;

    return (ambient + diffuse + specular);
}

vec3 CalcSpotlight(Spotlight light, vec3 norm, vec3 fragPos, vec3 viewDir)
{
   vec3 lightDir = normalize(light.position - fragPos); 
   // diffuse shading
   float diff = max(dot(norm, lightDir), 0.0);
   // specular shading
   vec3 reflectDir = reflect(-lightDir, norm);
   float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
   // combine results
   vec3 ambient = light.ambient * texture(material.diffuse, TexCoords).rgb;
   vec3 diffuse = light.diffuse * diff * texture(material.diffuse, TexCoords).rgb;
   vec3 specular = light.specular * spec * texture(material.specular, TexCoords).rgb;  

   // spotlight
   float theta = dot(lightDir, normalize(-light.direction)); 
   float epsilon = (light.cutoff - light.outerCutoff);
   float intensity = clamp((theta - light.outerCutoff) / epsilon, 0.0, 1.0);
   diffuse  *= intensity;
   specular *= intensity;

   // attenuation
   float distance = length(light.position - FragPos);
   float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
   ambient *= attenuation;
   diffuse *= attenuation;
   specular *= attenuation;

   return (ambient + diffuse + specular);
}
