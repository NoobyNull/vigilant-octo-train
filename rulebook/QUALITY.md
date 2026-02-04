# Quality Assurance

## Code Quality Tools

### Required
- **Clang-Format** - Automatic formatting (run on save)
- **Clang-Tidy** - Static analysis
- Warnings as errors on all platforms

### Recommended
- **CppCheck** - Additional static analysis
- **Address Sanitizer** - Memory error detection (Debug builds)

## Testing Strategy

### Unit Tests
- Google Test framework
- Test core logic (loaders, parsers, algorithms)
- No UI testing (ImGui is immediate mode)

### Test Organization
```
tests/
├── loaders/        # File format tests
├── mesh/           # Mesh operations
├── gcode/          # G-code parsing
├── optimizer/      # Cut optimization
└── database/       # Database operations
```

### Test Data
- Small test files in `tests/data/`
- Include a few medium-sized files (10-50MB) for load testing
- Generate procedural test meshes for edge cases

## Cross-Platform Testing

Before any merge:
- [ ] Builds on Windows (MSVC)
- [ ] Builds on Linux (GCC)
- [ ] Builds on macOS (Clang)
- [ ] Tests pass on all platforms

## Code Review Checklist

- [ ] No platform-specific code outside `platform/`
- [ ] Uses `std::filesystem` for paths
- [ ] No raw new/delete
- [ ] Error cases handled
- [ ] No compiler warnings

## Commit Standards

Since this is AI-generated code:
- Atomic commits (one logical change)
- Clear commit messages describing the change
- Main branch must always build
- Tests must pass

## System Requirements Validation

Test on minimum spec systems periodically:
- [ ] Runs on 8GB RAM system
- [ ] Runs on Intel integrated graphics (OpenGL 3.3)
- [ ] Loads 500MB+ model without issues
- [ ] UI stays responsive during load (background thread)
