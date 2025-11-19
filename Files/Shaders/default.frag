#version 460

out vec4 fragColor;

in vec3 i_Position;
in vec3 i_Normal;
in vec3 i_Color;

uniform sampler2D u_Diffuse0;
uniform sampler2D u_Specular0;

uniform vec4 u_LightColor;
uniform vec3 u_LightPosition;
uniform vec3 u_CameraPosition;

//vec4 pointLight() {	
//	// used in two variables so I calculate it here to not have to do it twice
//	vec3 lightVec = lightPos - crntPos;
//
//	// intensity of light with respect to distance
//	float dist = length(lightVec);
//	float a = 3.0;
//	float b = 0.7;
//	float inten = 1.0f / (a * dist * dist + b * dist + 1.0f);
//
//	// ambient lighting
//	float ambient = 0.20f;
//
//	// diffuse lighting
//	vec3 normal = normalize(Normal);
//	vec3 lightDirection = normalize(lightVec);
//	float diffuse = max(dot(normal, lightDirection), 0.0f);
//
//	// specular lighting
//	float specularLight = 0.50f;
//	vec3 viewDirection = normalize(camPos - crntPos);
//	vec3 reflectionDirection = reflect(-lightDirection, normal);
//	float specAmount = pow(max(dot(viewDirection, reflectionDirection), 0.0f), 16);
//	float specular = specAmount * specularLight;
//
//	return (texture(diffuse0, texCoord) * (diffuse * inten + ambient) + texture(specular0, texCoord).r * specular * inten) * lightColor;
//}
//
//vec4 direcLight() {
//	// ambient lighting
//	float ambient = 0.20f;
//
//	// diffuse lighting
//	vec3 normal = normalize(Normal);
//	vec3 lightDirection = normalize(vec3(1.0f, 1.0f, 0.0f));
//	float diffuse = max(dot(normal, lightDirection), 0.0f);
//
//	// specular lighting
//	float specularLight = 0.50f;
//	vec3 viewDirection = normalize(camPos - crntPos);
//	vec3 reflectionDirection = reflect(-lightDirection, normal);
//	float specAmount = pow(max(dot(viewDirection, reflectionDirection), 0.0f), 16);
//	float specular = specAmount * specularLight;
//
//	return (texture(diffuse0, texCoord) * (diffuse + ambient) + texture(specular0, texCoord).r * specular) * lightColor;
//}
//
//vec4 spotLight() {
//	// controls how big the area that is lit up is
//	float outerCone = 0.90f;
//	float innerCone = 0.95f;
//
//	// ambient lighting
//	float ambient = 0.20f;
//
//	// diffuse lighting
//	vec3 normal = normalize(Normal);
//	vec3 lightDirection = normalize(lightPos - crntPos);
//	float diffuse = max(dot(normal, lightDirection), 0.0f);
//
//	// specular lighting
//	float specularLight = 0.50f;
//	vec3 viewDirection = normalize(camPos - crntPos);
//	vec3 reflectionDirection = reflect(-lightDirection, normal);
//	float specAmount = pow(max(dot(viewDirection, reflectionDirection), 0.0f), 16);
//	float specular = specAmount * specularLight;
//
//	// calculates the intensity of the crntPos based on its angle to the center of the light cone
//	float angle = dot(vec3(0.0f, -1.0f, 0.0f), -lightDirection);
//	float inten = clamp((angle - outerCone) / (innerCone - outerCone), 0.0f, 1.0f);
//
//	return (texture(diffuse0, texCoord) * (diffuse * inten + ambient) + texture(specular0, texCoord).r * specular * inten) * lightColor;
//}

void main() {
	fragColor = vec4(i_Normal, 1.0f);
}