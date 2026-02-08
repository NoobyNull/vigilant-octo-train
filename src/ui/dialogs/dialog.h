#pragma once

#include <functional>
#include <string>

namespace dw {

// Base class for modal dialogs
class Dialog {
  public:
    explicit Dialog(const std::string& title) : m_title(title) {}
    virtual ~Dialog() = default;

    // Show the dialog (call each frame when visible)
    virtual void render() = 0;

    // Dialog state
    bool isOpen() const { return m_open; }
    void open() { m_open = true; }
    void close() { m_open = false; }

    const std::string& title() const { return m_title; }

  protected:
    std::string m_title;
    bool m_open = false;
};

// Common dialog result types
enum class DialogResult { None, Ok, Cancel, Yes, No };

} // namespace dw
