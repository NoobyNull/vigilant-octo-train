#pragma once

namespace dw {

// Polished ImGui theme configuration
class Theme {
  public:
    // Apply the polished dark theme (default)
    static void applyDark();

    // Apply light theme variant
    static void applyLight();

    // Apply high contrast theme
    static void applyHighContrast();

    // Colors for consistent use in panels
    struct Colors {
        static constexpr unsigned int Primary = 0xFF6699CC;    // Blue accent
        static constexpr unsigned int Secondary = 0xFF8899AA;  // Muted blue
        static constexpr unsigned int Success = 0xFF77CC77;    // Green
        static constexpr unsigned int Warning = 0xFFCCAA55;    // Yellow/Orange
        static constexpr unsigned int Error = 0xFFCC6666;      // Red
        static constexpr unsigned int Background = 0xFF1E2124; // Dark background
        static constexpr unsigned int Surface = 0xFF2A2D31;    // Panel background
        static constexpr unsigned int Border = 0xFF3A3D41;     // Border color
        static constexpr unsigned int Text = 0xFFE8E8E8;       // Primary text
        static constexpr unsigned int TextDim = 0xFF808080;    // Secondary text
    };

    // Base style (spacing, rounding, borders) — called before ScaleAllSizes for DPI
    static void applyBaseStyle();
};

} // namespace dw
