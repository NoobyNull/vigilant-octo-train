#pragma once

#include "dialog.h"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace dw {

// File filter for dialogs
struct FileFilter {
    std::string name;        // e.g., "3D Models"
    std::string extensions;  // e.g., "*.stl;*.obj;*.3mf"
};

// File dialog mode
enum class FileDialogMode { Open, Save, SelectFolder };

// Simple file dialog (native dialogs not available, using ImGui)
class FileDialog : public Dialog {
public:
    FileDialog();
    ~FileDialog() override = default;

    void render() override;

    // Show open file dialog
    void showOpen(const std::string& title,
                  const std::vector<FileFilter>& filters,
                  std::function<void(const std::string&)> callback);

    // Show save file dialog
    void showSave(const std::string& title,
                  const std::vector<FileFilter>& filters,
                  const std::string& defaultName,
                  std::function<void(const std::string&)> callback);

    // Show folder selection dialog
    void showFolder(const std::string& title,
                    std::function<void(const std::string&)> callback);

    // Common filters
    static std::vector<FileFilter> modelFilters();
    static std::vector<FileFilter> projectFilters();
    static std::vector<FileFilter> archiveFilters();
    static std::vector<FileFilter> allFilters();

private:
    void refreshDirectory();
    bool matchesFilter(const std::string& filename) const;

    FileDialogMode m_mode = FileDialogMode::Open;
    std::string m_currentPath;
    std::string m_selectedFile;
    std::string m_inputFileName;
    std::vector<FileFilter> m_filters;
    int m_selectedFilter = 0;

    struct DirEntry {
        std::string name;
        bool isDirectory;
        uint64_t size;
    };
    std::vector<DirEntry> m_entries;

    std::function<void(const std::string&)> m_callback;
};

}  // namespace dw
