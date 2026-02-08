#include "shader.h"

#include "../core/utils/log.h"
#include "gl_utils.h"

namespace dw {

Shader::~Shader() {
    if (m_program != 0) {
        glDeleteProgram(m_program);
    }
}

Shader::Shader(Shader&& other) noexcept : m_program(other.m_program) {
    other.m_program = 0;
}

Shader& Shader::operator=(Shader&& other) noexcept {
    if (this != &other) {
        if (m_program != 0) {
            glDeleteProgram(m_program);
        }
        m_program = other.m_program;
        m_uniformCache = std::move(other.m_uniformCache);
        other.m_program = 0;
    }
    return *this;
}

bool Shader::compile(const std::string& vertexSource, const std::string& fragmentSource) {
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
    if (vertexShader == 0) {
        return false;
    }

    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
    if (fragmentShader == 0) {
        glDeleteShader(vertexShader);
        return false;
    }

    // Create program
    m_program = glCreateProgram();
    glAttachShader(m_program, vertexShader);
    glAttachShader(m_program, fragmentShader);
    glLinkProgram(m_program);

    // Check link status
    GLint success = 0;
    glGetProgramiv(m_program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(m_program, sizeof(infoLog), nullptr, infoLog);
        log::errorf("Shader", "Link error: %s", infoLog);
        glDeleteProgram(m_program);
        m_program = 0;
    }

    // Clean up shaders (they're linked into program now)
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return m_program != 0;
}

void Shader::bind() const {
    glUseProgram(m_program);
}

void Shader::unbind() const {
    glUseProgram(0);
}

GLuint Shader::compileShader(GLenum type, const std::string& source) {
    GLuint shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
        const char* typeStr = (type == GL_VERTEX_SHADER) ? "vertex" : "fragment";
        log::errorf("Shader", "Compile error (%s): %s", typeStr, infoLog);
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

GLint Shader::getUniformLocation(const std::string& name) {
    auto it = m_uniformCache.find(name);
    if (it != m_uniformCache.end()) {
        return it->second;
    }

    GLint location = glGetUniformLocation(m_program, name.c_str());
    // Only cache valid locations; -1 means not found, allow re-lookup
    if (location != -1) {
        m_uniformCache[name] = location;
    }
    return location;
}

void Shader::setInt(const std::string& name, int value) {
    glUniform1i(getUniformLocation(name), value);
}

void Shader::setFloat(const std::string& name, f32 value) {
    glUniform1f(getUniformLocation(name), value);
}

void Shader::setVec2(const std::string& name, const Vec2& value) {
    glUniform2f(getUniformLocation(name), value.x, value.y);
}

void Shader::setVec3(const std::string& name, const Vec3& value) {
    glUniform3f(getUniformLocation(name), value.x, value.y, value.z);
}

void Shader::setVec4(const std::string& name, const Vec4& value) {
    glUniform4f(getUniformLocation(name), value.x, value.y, value.z, value.w);
}

void Shader::setMat4(const std::string& name, const Mat4& value) {
    glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
}

} // namespace dw
