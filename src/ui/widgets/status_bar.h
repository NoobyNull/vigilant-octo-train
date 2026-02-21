#pragma once

// Digital Workshop - Status Bar Widget
// Bottom-anchored widget showing import progress during active import,
// general status when idle. Not a Panel subclass — rendered directly by UIManager.

#include <functional>

namespace dw {

// Forward declarations
struct ImportProgress;
struct LoadingState;

class StatusBar {
  public:
    StatusBar() = default;
    ~StatusBar() = default;

    // Render the status bar. Call each frame from UIManager.
    void render(const LoadingState* loadingState);

    // Set import progress reference (called when import starts)
    void setImportProgress(const ImportProgress* progress);

    // Clear import progress (called when batch completes)
    void clearImportProgress();

    // Set cancel callback (wired by Application to ImportQueue::cancel)
    void setOnCancel(std::function<void()> callback) { m_onCancel = std::move(callback); }

  private:
    const ImportProgress* m_progress = nullptr;
    std::function<void()> m_onCancel;
};

} // namespace dw
