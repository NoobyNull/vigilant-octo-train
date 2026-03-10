#pragma once

#include <string>

#include "panel.h"

namespace dw {

// Blank dockable panel used as a grouping container.
// Users dock other panels around/inside it to organize their workspace.
// When closed, the layout resets to defaults.
class GroupPanel : public Panel {
  public:
    explicit GroupPanel(int id);

    void render() override;

    // True if the user closed this panel via the X button this frame.
    bool wasClosed() const { return m_wasClosed; }

  private:
    bool m_wasClosed = false;
};

} // namespace dw
