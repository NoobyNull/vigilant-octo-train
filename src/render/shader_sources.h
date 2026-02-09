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

out vec4 FragColor;

void main() {
    vec3 normal = normalize(vNormal);
    vec3 lightDir = normalize(-uLightDir);

    // Ambient
    vec3 ambient = uAmbient * uObjectColor;

    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * uLightColor * uObjectColor;

    // Specular
    vec3 viewDir = normalize(uViewPos - vWorldPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), uShininess);
    vec3 specular = spec * uLightColor * 0.5;

    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, 1.0);
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
