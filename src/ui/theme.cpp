#include "theme.h"

#include <imgui.h>

namespace dw {

void Theme::applyBaseStyle(float fontSize) {
    ImGuiStyle& style = ImGui::GetStyle();

    // Spacing and sizing — derived from base font size for proportional scaling
    const float f = fontSize;
    style.WindowPadding = ImVec2(f * 0.625f, f * 0.625f);
    style.FramePadding = ImVec2(f * 0.5f, f * 0.3125f);
    style.CellPadding = ImVec2(f * 0.375f, f * 0.25f);
    style.ItemSpacing = ImVec2(f * 0.5f, f * 0.375f);
    style.ItemInnerSpacing = ImVec2(f * 0.375f, f * 0.25f);
    style.TouchExtraPadding = ImVec2(0.0f, 0.0f);
    style.IndentSpacing = f * 1.25f;
    style.ScrollbarSize = f * 0.875f;
    style.GrabMinSize = f * 0.75f;

    // Rounded corners — modernized for consistency
    style.WindowRounding = 6.0f;
    style.ChildRounding = 5.0f;
    style.FrameRounding = 5.0f;     // Increased from 4.0f for button/input consistency
    style.PopupRounding = 5.0f;
    style.ScrollbarRounding = 6.0f;
    style.GrabRounding = 4.0f;      // Increased from 3.0f for visual harmony
    style.LogSliderDeadzone = 4.0f;
    style.TabRounding = 5.0f;       // Increased from 4.0f for consistency

    // Subtle borders
    style.WindowBorderSize = 1.0f;
    style.ChildBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.TabBorderSize = 0.0f;

    // Alignment
    style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
    style.WindowMenuButtonPosition = ImGuiDir_Left;
    style.ColorButtonPosition = ImGuiDir_Right;
    style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
    style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

    // Anti-aliasing
    style.AntiAliasedLines = true;
    style.AntiAliasedLinesUseTex = true;
    style.AntiAliasedFill = true;
}

void Theme::applyDark() {
    applyBaseStyle();

    ImVec4* colors = ImGui::GetStyle().Colors;

    // Main backgrounds
    colors[ImGuiCol_WindowBg] = ImVec4(0.118f, 0.129f, 0.141f, 1.0f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.118f, 0.129f, 0.141f, 0.0f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.102f, 0.110f, 0.118f, 0.98f);

    // Text
    colors[ImGuiCol_Text] = ImVec4(0.910f, 0.910f, 0.910f, 1.0f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.500f, 0.500f, 0.500f, 1.0f);

    // Borders
    colors[ImGuiCol_Border] = ImVec4(0.227f, 0.239f, 0.255f, 1.0f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

    // Frames (input fields, etc.)
    colors[ImGuiCol_FrameBg] = ImVec4(0.165f, 0.176f, 0.192f, 1.0f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.200f, 0.212f, 0.228f, 1.0f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.235f, 0.247f, 0.263f, 1.0f);

    // Title bar
    colors[ImGuiCol_TitleBg] = ImVec4(0.094f, 0.102f, 0.110f, 1.0f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.118f, 0.129f, 0.141f, 1.0f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.094f, 0.102f, 0.110f, 1.0f);

    // Menu bar
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.118f, 0.129f, 0.141f, 1.0f);

    // Scrollbar
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.102f, 0.110f, 0.118f, 1.0f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.275f, 0.290f, 0.306f, 1.0f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.353f, 0.369f, 0.388f, 1.0f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.400f, 0.420f, 0.440f, 1.0f);

    // Accent elements
    colors[ImGuiCol_CheckMark] = ImVec4(0.400f, 0.600f, 0.800f, 1.0f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.400f, 0.600f, 0.800f, 1.0f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.500f, 0.700f, 0.900f, 1.0f);

    // Buttons — unified accent family
    colors[ImGuiCol_Button] = ImVec4(0.200f, 0.212f, 0.228f, 1.0f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.280f, 0.400f, 0.560f, 1.0f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.400f, 0.600f, 0.800f, 1.0f);

    // Headers (collapsible, selectable) — enhanced hover visibility
    colors[ImGuiCol_Header] = ImVec4(0.200f, 0.212f, 0.228f, 1.0f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.280f, 0.400f, 0.560f, 0.95f);  // Increased from 0.8f
    colors[ImGuiCol_HeaderActive] = ImVec4(0.400f, 0.600f, 0.800f, 1.0f);

    // Separator — enhanced clarity for panel hierarchy
    colors[ImGuiCol_Separator] = ImVec4(0.275f, 0.290f, 0.306f, 1.0f);   // Slightly brighter for visibility
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.400f, 0.600f, 0.800f, 0.95f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.400f, 0.600f, 0.800f, 1.0f);

    // Resize grip
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.200f, 0.212f, 0.228f, 0.5f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.400f, 0.600f, 0.800f, 0.7f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.400f, 0.600f, 0.800f, 1.0f);

    // Tabs
    colors[ImGuiCol_Tab] = ImVec4(0.145f, 0.157f, 0.169f, 1.0f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.400f, 0.600f, 0.800f, 0.8f);
    colors[ImGuiCol_TabActive] = ImVec4(0.220f, 0.350f, 0.500f, 1.0f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.118f, 0.129f, 0.141f, 1.0f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.165f, 0.200f, 0.250f, 1.0f);

    // Docking
    colors[ImGuiCol_DockingPreview] = ImVec4(0.400f, 0.600f, 0.800f, 0.7f);
    colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.118f, 0.129f, 0.141f, 1.0f);

    // Plots
    colors[ImGuiCol_PlotLines] = ImVec4(0.610f, 0.610f, 0.610f, 1.0f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.0f, 0.430f, 0.350f, 1.0f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.400f, 0.600f, 0.800f, 1.0f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.500f, 0.700f, 0.900f, 1.0f);

    // Tables
    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.165f, 0.176f, 0.192f, 1.0f);
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.275f, 0.290f, 0.306f, 1.0f);
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.200f, 0.212f, 0.228f, 1.0f);
    colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 0.03f);

    // Selection
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.400f, 0.600f, 0.800f, 0.35f);

    // Drag & Drop
    colors[ImGuiCol_DragDropTarget] = ImVec4(0.400f, 0.600f, 0.800f, 1.0f);

    // Navigation
    colors[ImGuiCol_NavHighlight] = ImVec4(0.400f, 0.600f, 0.800f, 1.0f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.7f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.8f, 0.8f, 0.8f, 0.2f);

    // Modal dimming
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.5f);
}

void Theme::applyLight() {
    applyBaseStyle();

    ImVec4* colors = ImGui::GetStyle().Colors;

    // Main backgrounds — warm off-white
    colors[ImGuiCol_WindowBg] = ImVec4(0.973f, 0.973f, 0.980f, 1.0f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.973f, 0.973f, 0.980f, 0.0f);
    colors[ImGuiCol_PopupBg] = ImVec4(1.0f, 1.0f, 1.0f, 0.98f);

    // Text
    colors[ImGuiCol_Text] = ImVec4(0.102f, 0.102f, 0.180f, 1.0f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.420f, 0.450f, 0.500f, 1.0f);

    // Borders — light gray
    colors[ImGuiCol_Border] = ImVec4(0.886f, 0.894f, 0.910f, 1.0f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

    // Frames
    colors[ImGuiCol_FrameBg] = ImVec4(0.941f, 0.945f, 0.953f, 1.0f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.898f, 0.906f, 0.918f, 1.0f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.820f, 0.835f, 0.855f, 1.0f);

    // Title bar
    colors[ImGuiCol_TitleBg] = ImVec4(0.937f, 0.937f, 0.945f, 1.0f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.910f, 0.910f, 0.925f, 1.0f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.950f, 0.950f, 0.955f, 1.0f);

    // Menu bar
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.950f, 0.950f, 0.957f, 1.0f);

    // Scrollbar
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.960f, 0.960f, 0.965f, 1.0f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.780f, 0.790f, 0.810f, 1.0f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.650f, 0.665f, 0.690f, 1.0f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.530f, 0.545f, 0.570f, 1.0f);

    // Accent elements — blue
    colors[ImGuiCol_CheckMark] = ImVec4(0.200f, 0.400f, 0.700f, 1.0f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.200f, 0.400f, 0.700f, 1.0f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.150f, 0.350f, 0.650f, 1.0f);

    // Buttons
    colors[ImGuiCol_Button] = ImVec4(0.910f, 0.918f, 0.930f, 1.0f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.750f, 0.830f, 0.930f, 1.0f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.200f, 0.400f, 0.700f, 1.0f);

    // Headers — enhanced hover visibility
    colors[ImGuiCol_Header] = ImVec4(0.910f, 0.918f, 0.930f, 1.0f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.750f, 0.830f, 0.930f, 0.95f);  // Increased from 0.8f
    colors[ImGuiCol_HeaderActive] = ImVec4(0.200f, 0.400f, 0.700f, 1.0f);

    // Separator
    colors[ImGuiCol_Separator] = ImVec4(0.886f, 0.894f, 0.910f, 1.0f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.200f, 0.400f, 0.700f, 0.8f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.200f, 0.400f, 0.700f, 1.0f);

    // Resize grip
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.780f, 0.790f, 0.810f, 0.5f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.200f, 0.400f, 0.700f, 0.7f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.200f, 0.400f, 0.700f, 1.0f);

    // Tabs
    colors[ImGuiCol_Tab] = ImVec4(0.937f, 0.937f, 0.945f, 1.0f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.750f, 0.830f, 0.930f, 0.8f);
    colors[ImGuiCol_TabActive] = ImVec4(0.850f, 0.900f, 0.960f, 1.0f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.950f, 0.950f, 0.955f, 1.0f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.910f, 0.930f, 0.960f, 1.0f);

    // Docking
    colors[ImGuiCol_DockingPreview] = ImVec4(0.200f, 0.400f, 0.700f, 0.7f);
    colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.960f, 0.960f, 0.965f, 1.0f);

    // Plots
    colors[ImGuiCol_PlotLines] = ImVec4(0.400f, 0.400f, 0.400f, 1.0f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.900f, 0.350f, 0.280f, 1.0f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.200f, 0.400f, 0.700f, 1.0f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.150f, 0.350f, 0.650f, 1.0f);

    // Tables
    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.920f, 0.925f, 0.935f, 1.0f);
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.820f, 0.830f, 0.850f, 1.0f);
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.886f, 0.894f, 0.910f, 1.0f);
    colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.0f, 0.0f, 0.0f, 0.025f);

    // Selection
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.200f, 0.400f, 0.700f, 0.25f);

    // Drag & Drop
    colors[ImGuiCol_DragDropTarget] = ImVec4(0.200f, 0.400f, 0.700f, 1.0f);

    // Navigation
    colors[ImGuiCol_NavHighlight] = ImVec4(0.200f, 0.400f, 0.700f, 1.0f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.0f, 0.0f, 0.0f, 0.7f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.2f, 0.2f, 0.2f, 0.2f);

    // Modal dimming
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.4f);
}

void Theme::applyHighContrast() {
    applyBaseStyle();

    ImVec4* colors = ImGui::GetStyle().Colors;

    // Black background, white text, bright blue accent
    colors[ImGuiCol_WindowBg] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.98f);

    // Text — pure white for maximum contrast
    colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.600f, 0.600f, 0.600f, 1.0f);

    // Borders — bright white for visibility
    colors[ImGuiCol_Border] = ImVec4(1.0f, 1.0f, 1.0f, 0.5f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

    // Frames
    colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.35f, 0.35f, 0.35f, 1.0f);

    // Title bar
    colors[ImGuiCol_TitleBg] = ImVec4(0.05f, 0.05f, 0.05f, 1.0f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.10f, 0.10f, 0.10f, 1.0f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.05f, 0.05f, 0.05f, 1.0f);

    // Menu bar
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.05f, 0.05f, 0.05f, 1.0f);

    // Scrollbar
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.05f, 0.05f, 0.05f, 1.0f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.40f, 0.40f, 0.40f, 1.0f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.60f, 0.60f, 0.60f, 1.0f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.80f, 0.80f, 0.80f, 1.0f);

    // Accent — bright blue
    colors[ImGuiCol_CheckMark] = ImVec4(0.4f, 0.7f, 1.0f, 1.0f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.4f, 0.7f, 1.0f, 1.0f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.6f, 0.85f, 1.0f, 1.0f);

    // Buttons
    colors[ImGuiCol_Button] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.5f, 0.7f, 1.0f, 1.0f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.3f, 0.5f, 0.9f, 1.0f);

    // Headers — maximum contrast for accessibility
    colors[ImGuiCol_Header] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.4f, 0.6f, 0.9f, 1.0f);   // Increased from 0.8f for full opacity
    colors[ImGuiCol_HeaderActive] = ImVec4(0.4f, 0.7f, 1.0f, 1.0f);

    // Separator
    colors[ImGuiCol_Separator] = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.4f, 0.7f, 1.0f, 0.8f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.4f, 0.7f, 1.0f, 1.0f);

    // Resize grip
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.3f, 0.3f, 0.3f, 0.5f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.4f, 0.7f, 1.0f, 0.7f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.4f, 0.7f, 1.0f, 1.0f);

    // Tabs
    colors[ImGuiCol_Tab] = ImVec4(0.10f, 0.10f, 0.10f, 1.0f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.4f, 0.7f, 1.0f, 0.8f);
    colors[ImGuiCol_TabActive] = ImVec4(0.2f, 0.4f, 0.7f, 1.0f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.05f, 0.05f, 0.05f, 1.0f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.15f, 0.25f, 0.40f, 1.0f);

    // Docking
    colors[ImGuiCol_DockingPreview] = ImVec4(0.4f, 0.7f, 1.0f, 0.7f);
    colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);

    // Plots
    colors[ImGuiCol_PlotLines] = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.0f, 0.4f, 0.35f, 1.0f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.4f, 0.7f, 1.0f, 1.0f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.6f, 0.85f, 1.0f, 1.0f);

    // Tables
    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
    colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 0.04f);

    // Selection
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.4f, 0.7f, 1.0f, 0.35f);

    // Drag & Drop
    colors[ImGuiCol_DragDropTarget] = ImVec4(0.4f, 0.7f, 1.0f, 1.0f);

    // Navigation
    colors[ImGuiCol_NavHighlight] = ImVec4(0.4f, 0.7f, 1.0f, 1.0f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.7f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.8f, 0.8f, 0.8f, 0.2f);

    // Modal dimming
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.7f);
}

} // namespace dw
