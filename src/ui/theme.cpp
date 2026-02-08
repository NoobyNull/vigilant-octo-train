#include "theme.h"

#include <imgui.h>

namespace dw {

void Theme::applyBaseStyle() {
    ImGuiStyle& style = ImGui::GetStyle();

    // Refined spacing and sizing
    style.WindowPadding = ImVec2(10.0f, 10.0f);
    style.FramePadding = ImVec2(8.0f, 5.0f);
    style.CellPadding = ImVec2(6.0f, 4.0f);
    style.ItemSpacing = ImVec2(8.0f, 6.0f);
    style.ItemInnerSpacing = ImVec2(6.0f, 4.0f);
    style.TouchExtraPadding = ImVec2(0.0f, 0.0f);
    style.IndentSpacing = 20.0f;
    style.ScrollbarSize = 14.0f;
    style.GrabMinSize = 12.0f;

    // Polished rounded corners
    style.WindowRounding = 6.0f;
    style.ChildRounding = 4.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 6.0f;
    style.GrabRounding = 3.0f;
    style.LogSliderDeadzone = 4.0f;
    style.TabRounding = 4.0f;

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

    // Checkmark, slider grab - accent color
    colors[ImGuiCol_CheckMark] = ImVec4(0.400f, 0.600f, 0.800f, 1.0f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.400f, 0.600f, 0.800f, 1.0f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.500f, 0.700f, 0.900f, 1.0f);

    // Buttons
    colors[ImGuiCol_Button] = ImVec4(0.200f, 0.212f, 0.228f, 1.0f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.300f, 0.400f, 0.550f, 1.0f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.400f, 0.600f, 0.800f, 1.0f);

    // Headers (collapsible, selectable)
    colors[ImGuiCol_Header] = ImVec4(0.200f, 0.212f, 0.228f, 1.0f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.300f, 0.400f, 0.550f, 0.8f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.400f, 0.600f, 0.800f, 1.0f);

    // Separator
    colors[ImGuiCol_Separator] = ImVec4(0.227f, 0.239f, 0.255f, 1.0f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.400f, 0.600f, 0.800f, 0.8f);
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
    ImGui::StyleColorsLight();

    ImVec4* colors = ImGui::GetStyle().Colors;

    // Adjust accent color
    colors[ImGuiCol_CheckMark] = ImVec4(0.2f, 0.4f, 0.7f, 1.0f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.2f, 0.4f, 0.7f, 1.0f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.2f, 0.4f, 0.7f, 1.0f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.2f, 0.4f, 0.7f, 1.0f);
}

void Theme::applyHighContrast() {
    applyBaseStyle();

    ImVec4* colors = ImGui::GetStyle().Colors;

    // High contrast dark theme
    colors[ImGuiCol_WindowBg] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
    colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    colors[ImGuiCol_Border] = ImVec4(1.0f, 1.0f, 1.0f, 0.5f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
    colors[ImGuiCol_Button] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.5f, 0.7f, 1.0f, 1.0f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.3f, 0.5f, 0.9f, 1.0f);
}

} // namespace dw
