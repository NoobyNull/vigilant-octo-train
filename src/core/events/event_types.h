#pragma once

#include <cstdint>
#include <string>

namespace dw {

// Event type definitions for subsystem communication
// These are plain data structs (no inheritance required)

struct WorkspaceChanged {
    int64_t newModelId;
    std::string modelName;
};

struct ImportCompleted {
    int64_t modelId;
    std::string name;
};

struct ImportFailed {
    std::string filePath;
    std::string error;
};

struct ConfigFileChanged {};

struct ModelSelected {
    int64_t modelId;
};

struct ModelOpened {
    int64_t modelId;
};

} // namespace dw
