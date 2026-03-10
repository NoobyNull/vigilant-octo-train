#pragma once

namespace dw {
namespace shaders {

// Basic mesh shader with lighting
constexpr const char* MESH_VERTEX = R"(
#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform mat3 uNormalMatrix;

out vec3 vWorldPos;
out vec3 vNormal;
out vec2 vTexCoord;

void main() {
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    vWorldPos = worldPos.xyz;
    vNormal = uNormalMatrix * aNormal;
    vTexCoord = aTexCoord;
    gl_Position = uProjection * uView * worldPos;
}
)";

constexpr const char* MESH_FRAGMENT = R"(
#version 330 core

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vTexCoord;

uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform vec3 uAmbient;
uniform vec3 uObjectColor;
uniform vec3 uViewPos;
uniform float uShininess;
uniform bool uIsToolpath;
uniform sampler2D uMaterialTexture;
uniform bool uUseTexture;

out vec4 FragColor;

void main() {
    vec3 normal = normalize(vNormal);
    vec3 lightDir = normalize(-uLightDir);
    vec3 viewDir = normalize(uViewPos - vWorldPos);

    vec3 objectColor;

    // Toolpath mode overrides everything: blend cutting (blue-green) and rapid (orange-red)
    if (uIsToolpath) {
        vec3 cuttingColor = vec3(0.2, 0.6, 1.0);  // Blue for cutting moves (G1)
        vec3 rapidColor = vec3(1.0, 0.4, 0.1);     // Orange-red for rapid moves (G0)
        objectColor = mix(cuttingColor, rapidColor, vTexCoord.x);
    } else if (uUseTexture) {
        // Texture mode: sample wood grain texture
        objectColor = texture(uMaterialTexture, vTexCoord).rgb;
    } else {
        // Solid color fallback
        objectColor = uObjectColor;
    }

    // Hemisphere ambient: blend between sky (uAmbient) and dimmer ground color
    // based on how much the normal faces upward vs downward
    float hemiBlend = normal.y * 0.5 + 0.5;
    vec3 groundAmbient = uAmbient * 0.5;
    vec3 ambient = mix(groundAmbient, uAmbient, hemiBlend) * objectColor;

    // Half-Lambert wrap diffuse: maps [-1,1] -> [0,1] so shadow faces still
    // receive some diffuse light, preventing pure-black areas
    float NdotL = dot(normal, lightDir);
    float diff = NdotL * 0.5 + 0.5;
    diff *= diff;  // Square for softer falloff
    vec3 diffuse = diff * uLightColor * objectColor;

    // Fill light from camera direction (subtle rim/view light)
    float fillNdotV = max(dot(normal, viewDir), 0.0);
    vec3 fill = fillNdotV * 0.15 * uLightColor * objectColor;

    // Specular (Blinn-Phong)
    vec3 halfVec = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfVec), 0.0), uShininess);
    // Only apply specular on lit side
    spec *= smoothstep(0.0, 0.1, max(NdotL, 0.0));
    vec3 specular = spec * uLightColor * 0.5;

    vec3 result = ambient + diffuse + fill + specular;

    // Slight transparency for rapid moves in toolpath mode
    float alpha = 1.0;
    if (uIsToolpath && vTexCoord.x > 0.5) {
        alpha = 0.8;
    }

    FragColor = vec4(result, alpha);
}
)";

// Simple flat color shader
constexpr const char* FLAT_VERTEX = R"(
#version 330 core

layout (location = 0) in vec3 aPos;

uniform mat4 uMVP;

void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
}
)";

constexpr const char* FLAT_FRAGMENT = R"(
#version 330 core

uniform vec4 uColor;

out vec4 FragColor;

void main() {
    FragColor = uColor;
}
)";

// Height-colored line shader (for G-code depth visualization)
constexpr const char* HEIGHT_LINE_VERTEX = R"(
#version 330 core

layout (location = 0) in vec3 aPos;

uniform mat4 uMVP;
uniform float uYMin;   // min Y (renderer space) = G-code Z bottom
uniform float uYMax;   // max Y (renderer space) = G-code Z top

out float vHeight;     // 0..1 normalized height

void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
    float range = uYMax - uYMin;
    vHeight = (range > 0.001) ? clamp((aPos.y - uYMin) / range, 0.0, 1.0) : 0.5;
}
)";

constexpr const char* HEIGHT_LINE_FRAGMENT = R"(
#version 330 core

in float vHeight;

uniform vec4 uColorLow;   // Color at bottom (deepest cut)
uniform vec4 uColorHigh;  // Color at top (shallowest / surface)

out vec4 FragColor;

void main() {
    FragColor = mix(uColorLow, uColorHigh, vHeight);
}
)";

// Grid shader
constexpr const char* GRID_VERTEX = R"(
#version 330 core

layout (location = 0) in vec3 aPos;

uniform mat4 uMVP;

out vec3 vWorldPos;

void main() {
    vWorldPos = aPos;
    gl_Position = uMVP * vec4(aPos, 1.0);
}
)";

constexpr const char* GRID_FRAGMENT = R"(
#version 330 core

in vec3 vWorldPos;

uniform vec4 uColor;
uniform float uFadeStart;
uniform float uFadeEnd;

out vec4 FragColor;

void main() {
    float dist = length(vWorldPos.xz);
    float fade = 1.0 - smoothstep(uFadeStart, uFadeEnd, dist);
    FragColor = vec4(uColor.rgb, uColor.a * fade);
}
)";

} // namespace shaders
} // namespace dw
