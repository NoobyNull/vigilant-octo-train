#include "texture.h"

#include <utility>

#include "../core/utils/log.h"

namespace dw {

// ---------------------------------------------------------------------------
// Destructor / move semantics
// ---------------------------------------------------------------------------

Texture::~Texture() {
    if (m_id != 0) {
        glDeleteTextures(1, &m_id);
        m_id = 0;
    }
}

Texture::Texture(Texture&& other) noexcept
    : m_id(other.m_id), m_width(other.m_width), m_height(other.m_height) {
    other.m_id = 0;
    other.m_width = 0;
    other.m_height = 0;
}

Texture& Texture::operator=(Texture&& other) noexcept {
    if (this != &other) {
        // Release current resource
        if (m_id != 0) {
            glDeleteTextures(1, &m_id);
        }
        m_id = other.m_id;
        m_width = other.m_width;
        m_height = other.m_height;
        other.m_id = 0;
        other.m_width = 0;
        other.m_height = 0;
    }
    return *this;
}

// ---------------------------------------------------------------------------
// upload()
// ---------------------------------------------------------------------------

bool Texture::upload(const uint8_t* data, int width, int height, GLenum format) {
    if (!data || width <= 0 || height <= 0) {
        log::error("Texture", "upload() called with invalid arguments");
        return false;
    }

    // Release any existing texture
    if (m_id != 0) {
        glDeleteTextures(1, &m_id);
        m_id = 0;
    }

    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);

    // Set default wrap (GL_REPEAT for tileable wood grain textures)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // Set default filter (trilinear with mipmaps)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload pixel data
    glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(format), width, height, 0, format,
                 GL_UNSIGNED_BYTE, data);

    // Generate mipmaps
    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);

    m_width = width;
    m_height = height;

    return true;
}

// ---------------------------------------------------------------------------
// bind() / unbind()
// ---------------------------------------------------------------------------

void Texture::bind(unsigned int slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, m_id);
}

void Texture::unbind() const {
    glBindTexture(GL_TEXTURE_2D, 0);
}

// ---------------------------------------------------------------------------
// setWrap() / setFilter()
// ---------------------------------------------------------------------------

void Texture::setWrap(GLenum wrap) {
    if (m_id == 0) return;
    glBindTexture(GL_TEXTURE_2D, m_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, static_cast<GLint>(wrap));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, static_cast<GLint>(wrap));
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::setFilter(GLenum minFilter, GLenum magFilter) {
    if (m_id == 0) return;
    glBindTexture(GL_TEXTURE_2D, m_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(minFilter));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(magFilter));
    glBindTexture(GL_TEXTURE_2D, 0);
}

} // namespace dw
