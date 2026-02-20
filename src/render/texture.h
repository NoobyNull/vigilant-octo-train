#pragma once

#include <glad/gl.h>

#include "../core/types.h"

namespace dw {

// RAII wrapper for an OpenGL 2D texture.
//
// Lifecycle:
//   - Default-constructed: invalid (no GPU resources allocated)
//   - upload(): allocates texture, uploads pixel data, generates mipmaps
//   - Destructor: calls glDeleteTextures if texture was ever uploaded
//
// Move semantics: transfers GPU handle; source becomes invalid.
// Copy is deleted — GPU objects cannot be trivially copied.
class Texture {
  public:
    Texture() = default;
    ~Texture();

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;

    // Upload pixel data to GPU.
    // data must point to width * height * 4 bytes of RGBA data.
    // Default format: GL_RGBA. Returns true on success.
    // Default wrap: GL_REPEAT (tileable); default filter: LINEAR_MIPMAP_LINEAR/LINEAR.
    bool upload(const uint8_t* data, int width, int height, GLenum format = GL_RGBA);

    // Bind this texture to the given texture unit (slot 0 by default)
    void bind(unsigned int slot = 0) const;

    // Unbind texture unit 0 (or the given slot)
    void unbind() const;

    // Set wrap mode for both S and T axes (e.g. GL_REPEAT, GL_CLAMP_TO_EDGE)
    void setWrap(GLenum wrap);

    // Set minification and magnification filters
    // Typical: GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR
    void setFilter(GLenum minFilter, GLenum magFilter);

    // Query
    bool isValid() const { return m_id != 0; }
    GLuint id() const { return m_id; }
    int width() const { return m_width; }
    int height() const { return m_height; }

  private:
    GLuint m_id = 0;
    int m_width = 0;
    int m_height = 0;
};

} // namespace dw
