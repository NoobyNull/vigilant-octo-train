#pragma once

#include <optional>
#include <string>
#include <unordered_map>

#include <glad/gl.h>

#include "../core/types.h"

namespace dw {

class Shader {
  public:
    Shader() = default;
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    // Create shader from source
    [[nodiscard]] bool compile(const std::string& vertexSource, const std::string& fragmentSource);

    // Use this shader
    void bind() const;
    void unbind() const;

    // Check if valid
    bool isValid() const { return m_program != 0; }
    GLuint handle() const { return m_program; }

    // Uniform setters
    void setBool(const std::string& name, bool value);
    void setInt(const std::string& name, int value);
    void setFloat(const std::string& name, f32 value);
    void setVec2(const std::string& name, const Vec2& value);
    void setVec3(const std::string& name, const Vec3& value);
    void setVec4(const std::string& name, const Vec4& value);
    void setMat3(const std::string& name, const glm::mat3& value);
    void setMat4(const std::string& name, const Mat4& value);

  private:
    GLint getUniformLocation(const std::string& name);
    GLuint compileShader(GLenum type, const std::string& source);

    GLuint m_program = 0;
    mutable std::unordered_map<std::string, std::optional<GLint>> m_uniformCache;
};

} // namespace dw
