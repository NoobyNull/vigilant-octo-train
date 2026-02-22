#pragma once

// Digital Workshop - Toast Notification System
// Self-contained ImGui overlay for transient notifications.
// Toasts appear in top-right corner, auto-dismiss after duration, and fade out.

#include <string>
#include <vector>

namespace dw {

enum class ToastType { Info, Warning, Error, Success };

struct Toast {
    ToastType type;
    std::string title;
    std::string message;
    float duration;    // seconds
    float elapsed = 0; // seconds since creation
    float opacity = 1.0f;
};

class ToastManager {
  public:
    static ToastManager& instance();

    // Show a toast notification
    void show(ToastType type,
              const std::string& title,
              const std::string& message = "",
              float duration = 3.0f);

    // Call each frame from UIManager to render active toasts
    void render(float deltaTime);

  private:
    ToastManager() = default;

    std::vector<Toast> m_toasts;
    static constexpr int MAX_VISIBLE = 5;
    static constexpr float FADE_DURATION = 0.5f;

    // Rate limiting for batch errors
    int m_errorCountThisBatch = 0;
    bool m_rateLimitActive = false;
    static constexpr int ERROR_TOAST_LIMIT = 10;

  public:
    // Reset rate limiting when a new batch starts
    void resetBatch() {
        m_errorCountThisBatch = 0;
        m_rateLimitActive = false;
    }
};

} // namespace dw
