#version 430
/*
	File Name	: color.vert
	Author		: Bora Yalciner
	Description	:

		Basic fragment shader that just outputs
		color to the FBO
*/


// Definitions
// These locations must match between vertex/fragment shaders
#define IN_UV        layout(location = 0)
#define IN_NORMAL    layout(location = 1)
#define IN_COLOR     layout(location = 2)
#define IN_WORLD_POS layout(location = 3)
#define IN_DEPTH     layout(location = 4) // for shadows

// This output must match to the COLOR_ATTACHMENTi (where 'i' is this location)
#define OUT_FBO      layout(location = 0)

// This must match GL_TEXTUREi (where 'i' is this binding)
#define T_ALBEDO     layout(binding = 0)
#define T_SHADOWMAP  layout(binding = 1) // shadow mapping
#define T_NIGHT_MAP  layout(binding = 2)
#define T_CLOUD      layout(binding = 3)
#define T_SPEC_MAP   layout(binding = 4)

// This must match the first parameter of glUniform...() calls
#define U_MODE       layout(location = 0) //Render mode 0,1,2 or 3
#define U_SUN_DIR    layout(location = 1) //Sun direction
#define U_CAMERA_POS layout(location = 2) //For specular
#define U_LIGHT_MAT  layout(location = 5) //Light matrix
#define U_USE_NIGHT  layout(location = 6)


// Input
in IN_UV        vec2 fUV;
in IN_NORMAL    vec3 fNormal;
in IN_WORLD_POS vec3 fWorldPos;
in IN_DEPTH     float fDepth;

// Output
// This parameter goes to the framebuffer
out OUT_FBO vec4 fboColor;

// Uniforms
U_MODE       uniform uint uMode;
U_SUN_DIR    uniform vec3 uSunDir;
U_LIGHT_MAT  uniform mat4 uLightSpaceMatrix;
U_CAMERA_POS uniform vec3 uCameraPos;
U_USE_NIGHT  uniform uint uUseNightMap;

// Textures
uniform T_ALBEDO    sampler2D tAlbedo;
uniform T_SHADOWMAP sampler2D tShadowMap;
uniform T_NIGHT_MAP sampler2D tNightMap;
uniform T_CLOUD     sampler2D tCloud;
uniform T_SPEC_MAP  sampler2D tSpecMap;
void main(void)
{
    
    // Shadow check
    if (uMode == 3) {
        fboColor = vec4(fDepth, 0.0, 0.0, 1.0); 
        return;
    }

    // Sun / Just white
    if (uMode == 0) {
        fboColor = vec4(1.0, 1.0, 1.0, 1.0);
        return;
    }

    
    vec4 texColor = texture(tAlbedo, fUV);

    // No shading/shadowing
    if (uMode == 1) {
        fboColor = texColor;
        return;
    }
    if(uMode == 4)
    {
        vec4 c = texture(tCloud, fUV);

        if(c.a < 0.05)
            discard;

            vec3 N = normalize(fNormal);
        vec3 L = normalize(uSunDir);

        float diff = max(dot(N, L), 0.0);
        float ambient = 0.4;
        float intensity = ambient + diff*0.6;

        vec3 rgb = c.rgb * intensity;

        fboColor = vec4(rgb, c.a);
        return;
    }

    // Default rendering
    if (uMode == 2) {
        vec3 normal   = normalize(fNormal);
        vec3 lightDir = normalize(uSunDir);
        vec3 viewDir  = normalize(uCameraPos - fWorldPos);
        vec3 halfDir  = normalize(lightDir + viewDir);

        float NdotL = max(dot(normal, lightDir), 0.0);
        float NdotH = max(dot(normal, halfDir), 0.0);


        float s = texture(tSpecMap, fUV).r;


        float lowShininess  = 8.0;
        float highShininess = 128.0;
        float shininess     = mix(lowShininess, highShininess, s);


        float ks = 0.6;

        float spec = 0.0;
        if (NdotL > 0.0 && NdotH > 0.0) {

            spec = ks * pow(NdotH, shininess) * NdotL;
        }

        vec3 specularColor = vec3(0.8, 0.9, 1.0) * spec;



        vec4 fragPosLightSpace = uLightSpaceMatrix * vec4(fWorldPos, 1.0);
        vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
        vec3 shadowUV   = projCoords * 0.5 + 0.5;

        float currentDepth = projCoords.z;
        float shadow = 0.0;
        float bias   = max(0.005 * (1.0 - dot(normal, lightDir)), 0.0005);

        if (projCoords.z > 1.0 ||
            shadowUV.x < 0.0 || shadowUV.x > 1.0 ||
            shadowUV.y < 0.0 || shadowUV.y > 1.0) {

            shadow = 0.0;
            } else {
                vec2 texelSize = 1.0 / textureSize(tShadowMap, 0);
                for (int x = -2; x <= 2; ++x) {
                    for (int y = -2; y <= 2; ++y) {
                        float pcfDepth = texture(tShadowMap,
                                                 shadowUV.xy + vec2(x, y) * texelSize).r;
                                                 shadow += (currentDepth - bias) > pcfDepth ? 1.0 : 0.0;
                    }
                }
                shadow /= 25.0;
            }


            vec3 texColor = texture(tAlbedo, fUV).rgb;

            float ambientStrength = 0.15;
            vec3 ambientLight     = ambientStrength * vec3(1.0);


            float diffuseScale    = 1.0;

            vec3 diffuseLight =
            (1.0 - shadow) * NdotL * vec3(1.0) * diffuseScale;

            vec3 specularLight =
            (1.0 - shadow) * specularColor;

            vec3 base = (ambientLight + diffuseLight) * texColor + specularLight;
            vec3 result = base;
            if (uUseNightMap == 1u) {
                vec3 nightColor   = texture(tNightMap, fUV).rgb * 1.5;
                float nightFactor = 1.0 - smoothstep(0.05, 0.25, NdotL);
                result += nightColor * nightFactor;
            }

            fboColor = vec4(result, 1.0);
    }

    if (uMode == 5) {
        vec3 normal = normalize(fNormal);
        vec3 lightDir = normalize(uSunDir);
        
        float ambient = 0.05; 
        float diff = max(dot(normal, lightDir), 0.0);

        vec3 viewDir = normalize(uCameraPos - fWorldPos);
        vec3 reflectDir = reflect(-lightDir, normal);  

        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 8.0f);
        vec3 specularColor = vec3(0.2) * spec; 

        vec4 fragPosLightSpace = uLightSpaceMatrix * vec4(fWorldPos, 1.0);
        vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
        vec3 shadowUV = projCoords * 0.5 + 0.5;

        float currentDepth = projCoords.z;
        float shadow = 0.0;
        float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.0005); 

        if (projCoords.z > 1.0 || shadowUV.x < 0.0 || shadowUV.x > 1.0 || shadowUV.y < 0.0 || shadowUV.y > 1.0) {
            shadow = 0.0; 
        } else { 
            vec2 texelSize = 1.0 / textureSize(tShadowMap, 0);
            for(int x = -2; x <= 2; ++x) {
                for(int y = -2; y <= 2; ++y) {
                    float pcfDepth = texture(tShadowMap, shadowUV.xy + vec2(x, y) * texelSize).r;
                    shadow += (currentDepth - bias) > pcfDepth ? 1.0 : 0.0;
                }
            }
            shadow /= 25.0;
        }


        float ambientStrength = 0.15; 
        vec3 ambientLight = ambientStrength * vec3(1.0);

        vec3 diffuseLight = (1.0 - shadow) * (diff * vec3(1.0));

        vec3 specularLight = (1.0 - shadow) * specularColor;
        
        vec3 finalLight = (ambientLight + diffuseLight); 
        vec3 result = finalLight * texColor.rgb + specularLight;

        fboColor = vec4(result, texColor.a);
    }
}
