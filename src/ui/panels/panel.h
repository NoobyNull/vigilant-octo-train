#pragma once

#include <string>

namespace dw {

// Base class for all UI panels
class Panel {
public:
    Panel(const std::string& title) : m_title(title) {}
    virtual ~Panel() = default;

    // Called each frame to render the panel
    virtual void render() = 0;

    // Panel state
    bool isOpen() const { return m_open; }
    void setOpen(bool open) { m_open = open; }
    void toggle() { m_open = !m_open; }

    const std::string& title() const { return m_title; }

protected:
    std::string m_title;
    bool m_open = true;
};

}  // namespace dw
