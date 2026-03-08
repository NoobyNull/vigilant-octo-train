#pragma once

#include <string>

#include "../types.h"

namespace dw {

// Pre-parse file validation results
struct ValidationResult {
    bool valid = false;
    std::string reason;
};

// Validates file content before import.
// - Mesh files (STL/OBJ/3MF): structural size check with tolerance
// - Text files (G-code/NC): deep content sampling to reject binary payloads
namespace file_validator {

// Validate a file buffer against its declared extension.
// Checks magic bytes, structural integrity, and size consistency.
ValidationResult validate(const ByteBuffer& data, const std::string& extension);

// Individual format validators
ValidationResult validateSTL(const ByteBuffer& data);
ValidationResult validateOBJ(const ByteBuffer& data);
ValidationResult validate3MF(const ByteBuffer& data);
ValidationResult validateGCode(const ByteBuffer& data);

} // namespace file_validator
} // namespace dw
