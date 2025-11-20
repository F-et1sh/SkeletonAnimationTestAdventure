#version 460

out vec4 fragColor;

in vec3 i_Position;
in vec2 i_TexCoord;
in mat3 i_TBN;

uniform sampler2D u_Diffuse0;
uniform sampler2D u_Specular0;
uniform sampler2D u_Normal0;

uniform vec4 u_LightColor;
uniform vec3 u_LightPosition;
uniform vec3 u_CameraPosition;

vec3 getNormal() {
    vec3 n = texture(u_Normal0, i_TexCoord).xyz;
    n = n * 2.0f - 1.0f;
    return normalize(i_TBN * n);
}

vec4 pointLight() {	
	// used in two variables so I calculate it here to not have to do it twice
	vec3 lightVec = u_LightPosition - i_Position;

	// intensity of light with respect to distance
	float dist = length(lightVec);
	float a = 3.0;
	float b = 0.7;
	float inten = 1.0f / (a * dist * dist + b * dist + 1.0f);

	// ambient lighting
	float ambient = 0.20f;

	// diffuse lighting
	vec3 normal = getNormal();
	vec3 lightDirection = normalize(lightVec);
	float diffuse = max(dot(normal, lightDirection), 0.0f);

	// specular lighting
	float specularLight = 0.50f;
	vec3 viewDirection = normalize(u_CameraPosition - i_Position);
	vec3 reflectionDirection = reflect(-lightDirection, normal);
	float specAmount = pow(max(dot(viewDirection, reflectionDirection), 0.0f), 16);
	float specular = specAmount * specularLight;

	return (texture(u_Diffuse0, i_TexCoord) * (diffuse * inten + ambient) + texture(u_Specular0, i_TexCoord).r * specular * inten) * u_LightColor;
}

vec4 direcLight() {
	// ambient lighting
	float ambient = 0.20f;

	// diffuse lighting
	vec3 normal = getNormal();
	vec3 lightDirection = normalize(vec3(1.0f, 1.0f, 0.0f));
	float diffuse = max(dot(normal, lightDirection), 0.0f);

	// specular lighting
	float specularLight = 0.50f;
	vec3 viewDirection = normalize(u_CameraPosition - i_Position);
	vec3 reflectionDirection = reflect(-lightDirection, normal);
	float specAmount = pow(max(dot(viewDirection, reflectionDirection), 0.0f), 16);
	float specular = specAmount * specularLight;

	return (texture(u_Diffuse0, i_TexCoord) * (diffuse + ambient) + texture(u_Specular0, i_TexCoord).r * specular) * u_LightColor;
}

vec4 spotLight() {
	// controls how big the area that is lit up is
	float outerCone = 0.90f;
	float innerCone = 0.95f;

	// ambient lighting
	float ambient = 0.20f;

	// diffuse lighting
	vec3 normal = getNormal();
	vec3 lightDirection = normalize(u_LightPosition - i_Position);
	float diffuse = max(dot(normal, lightDirection), 0.0f);

	// specular lighting
	float specularLight = 0.50f;
	vec3 viewDirection = normalize(u_CameraPosition - i_Position);
	vec3 reflectionDirection = reflect(-lightDirection, normal);
	float specAmount = pow(max(dot(viewDirection, reflectionDirection), 0.0f), 16);
	float specular = specAmount * specularLight;

	// calculates the intensity of the i_Position based on its angle to the center of the light cone
	float angle = dot(vec3(0.0f, -1.0f, 0.0f), -lightDirection);
	float inten = clamp((angle - outerCone) / (innerCone - outerCone), 0.0f, 1.0f);

	return (texture(u_Diffuse0, i_TexCoord) * (diffuse * inten + ambient) + texture(u_Specular0, i_TexCoord).r * specular * inten) * u_LightColor;
}

vec3 desaturate(vec3 color, float amount) {
    vec3 gray = vec3(dot(vec3(0.2126, 0.7152, 0.0722), color));
    return vec3(mix(color, gray, amount));
}

void main() {
	fragColor = direcLight();
}