#pragma once

// Icon definitions using Unicode characters
// These can be used with icon fonts like FontAwesome or Material Icons

namespace dw {
namespace icons {

// File operations
constexpr const char* ICON_NEW = "\xEF\x85\x9B";    //
constexpr const char* ICON_OPEN = "\xEF\x81\xBC";   //
constexpr const char* ICON_SAVE = "\xEF\x83\x87";   //
constexpr const char* ICON_IMPORT = "\xEF\x81\x9E"; //
constexpr const char* ICON_EXPORT = "\xEF\x81\x9D"; //

// View controls
constexpr const char* ICON_ZOOM_IN = "\xEF\x80\x8E";  //
constexpr const char* ICON_ZOOM_OUT = "\xEF\x80\x90"; //
constexpr const char* ICON_FIT = "\xEF\x82\xB5";      //
constexpr const char* ICON_GRID = "\xEF\x80\x8A";     //

// Actions
constexpr const char* ICON_ADD = "\xEF\x81\xA7";     //
constexpr const char* ICON_REMOVE = "\xEF\x82\x88";  //
constexpr const char* ICON_DELETE = "\xEF\x87\xB8";  //
constexpr const char* ICON_SEARCH = "\xEF\x80\x82";  //
constexpr const char* ICON_FILTER = "\xEF\x82\xB0";  //
constexpr const char* ICON_REFRESH = "\xEF\x80\xA1"; //

// Object types
constexpr const char* ICON_MODEL = "\xEF\x86\xB2";   //
constexpr const char* ICON_PROJECT = "\xEF\x81\xBC"; //
constexpr const char* ICON_FOLDER = "\xEF\x81\xBC";  //
constexpr const char* ICON_FILE = "\xEF\x85\x9B";    //

// Settings and status
constexpr const char* ICON_SETTINGS = "\xEF\x80\x93"; //
constexpr const char* ICON_INFO = "\xEF\x81\x9A";     //
constexpr const char* ICON_WARNING = "\xEF\x81\xB1";  //
constexpr const char* ICON_ERROR = "\xEF\x81\xAA";    //
constexpr const char* ICON_QUESTION = "\xEF\x81\x99"; //

// View modes
constexpr const char* ICON_LIST = "\xEF\x80\xBA"; //

// Without icon fonts, use text labels
namespace text {

constexpr const char* NEW = "[New]";
constexpr const char* OPEN = "[Open]";
constexpr const char* SAVE = "[Save]";
constexpr const char* IMPORT = "[Import]";
constexpr const char* EXPORT = "[Export]";
constexpr const char* ADD = "[+]";
constexpr const char* REMOVE = "[-]";
constexpr const char* DELETE = "[X]";
constexpr const char* SEARCH = "[?]";
constexpr const char* SETTINGS = "[S]";

} // namespace text
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
