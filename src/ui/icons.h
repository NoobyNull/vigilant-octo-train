#pragma once

// Icon definitions using text labels
// FontAwesome font is not loaded, so we use readable text labels instead

namespace dw {
namespace icons {

// File operations
constexpr const char* ICON_NEW = "[New]";
constexpr const char* ICON_OPEN = "[Open]";
constexpr const char* ICON_SAVE = "[Save]";
constexpr const char* ICON_IMPORT = "[Import]";
constexpr const char* ICON_EXPORT = "[Export]";

// View controls
constexpr const char* ICON_ZOOM_IN = "[+]";
constexpr const char* ICON_ZOOM_OUT = "[-]";
constexpr const char* ICON_FIT = "[Fit]";
constexpr const char* ICON_GRID = "[Grid]";

// Actions
constexpr const char* ICON_ADD = "[+]";
constexpr const char* ICON_REMOVE = "[-]";
constexpr const char* ICON_DELETE = "[X]";
constexpr const char* ICON_SEARCH = "[Search]";
constexpr const char* ICON_FILTER = "[Filter]";
constexpr const char* ICON_REFRESH = "[Refresh]";

// Object types
constexpr const char* ICON_MODEL = "[M]";
constexpr const char* ICON_PROJECT = "[P]";
constexpr const char* ICON_FOLDER = "[D]";
constexpr const char* ICON_FILE = "[F]";

// Settings and status
constexpr const char* ICON_SETTINGS = "[Settings]";
constexpr const char* ICON_INFO = "[i]";
constexpr const char* ICON_WARNING = "[!]";
constexpr const char* ICON_ERROR = "[X]";
constexpr const char* ICON_QUESTION = "[?]";

// View modes
constexpr const char* ICON_LIST = "[List]";

} // namespace icons

// Convenience struct for simpler access: Icons::Model instead of icons::ICON_MODEL
struct Icons {
    static constexpr const char* New = icons::ICON_NEW;
    static constexpr const char* Open = icons::ICON_OPEN;
    static constexpr const char* Save = icons::ICON_SAVE;
    static constexpr const char* Import = icons::ICON_IMPORT;
    static constexpr const char* Export = icons::ICON_EXPORT;
    static constexpr const char* ZoomIn = icons::ICON_ZOOM_IN;
    static constexpr const char* ZoomOut = icons::ICON_ZOOM_OUT;
    static constexpr const char* Fit = icons::ICON_FIT;
    static constexpr const char* Grid = icons::ICON_GRID;
    static constexpr const char* Add = icons::ICON_ADD;
    static constexpr const char* Remove = icons::ICON_REMOVE;
    static constexpr const char* Delete = icons::ICON_DELETE;
    static constexpr const char* Search = icons::ICON_SEARCH;
    static constexpr const char* Filter = icons::ICON_FILTER;
    static constexpr const char* Refresh = icons::ICON_REFRESH;
    static constexpr const char* Model = icons::ICON_MODEL;
    static constexpr const char* Project = icons::ICON_PROJECT;
    static constexpr const char* Folder = icons::ICON_FOLDER;
    static constexpr const char* File = icons::ICON_FILE;
    static constexpr const char* Settings = icons::ICON_SETTINGS;
    static constexpr const char* Info = icons::ICON_INFO;
    static constexpr const char* Warning = icons::ICON_WARNING;
    static constexpr const char* Error = icons::ICON_ERROR;
    static constexpr const char* Question = icons::ICON_QUESTION;
    static constexpr const char* List = icons::ICON_LIST;
};

} // namespace dw
