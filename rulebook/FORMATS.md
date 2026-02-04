# Supported File Formats

## 3D Model Formats

### STL (Stereolithography)

| Aspect | Details |
|--------|---------|
| Extensions | `.stl` |
| Variants | Binary, ASCII |
| Priority | Primary format |
| Features | Triangles only, no color/texture |

**Implementation Notes:**
- Binary STL is preferred (smaller, faster to parse)
- Auto-detect binary vs ASCII by checking header
- Validate triangle count vs file size for binary

**Performance:**
- Binary: ~1GB/s parse rate on modern CPU
- ASCII: ~100MB/s (text parsing overhead)

### OBJ (Wavefront)

| Aspect | Details |
|--------|---------|
| Extensions | `.obj` |
| Materials | `.mtl` (optional) |
| Priority | Primary format |
| Features | Vertices, normals, UVs, materials |

**Implementation Notes:**
- Support faces with 3+ vertices (triangulate quads/ngons)
- MTL parsing optional but supported
- Handle relative indices (negative values)

**Performance:**
- ~200MB/s parse rate (text format)

### 3MF (3D Manufacturing Format)

| Aspect | Details |
|--------|---------|
| Extensions | `.3mf` |
| Structure | ZIP archive with XML |
| Priority | Secondary format |
| Features | Mesh, colors, materials, metadata |

**Implementation Notes:**
- Use miniz or similar for ZIP extraction
- Parse XML for mesh data
- Extract thumbnail if present
- Handle multi-object files

**Performance:**
- Depends on compression ratio
- XML parsing adds overhead

## G-Code

| Aspect | Details |
|--------|---------|
| Extensions | `.gcode`, `.nc`, `.ngc` |
| Purpose | CNC/3D printer toolpaths |
| Features | Movement, timing, tool changes |

**Supported Commands:**
- `G0`, `G1` - Linear moves (rapid, feed)
- `G2`, `G3` - Arc moves (CW, CCW)
- `G20`, `G21` - Units (inch/mm)
- `M3`, `M5` - Spindle on/off
- `M6` - Tool change
- `F` - Feed rate
- Comments (`;` and `()`)

**Analysis Output:**
- Total path length
- Estimated time (based on feed rates)
- Tool change count
- Rapid vs cutting move breakdown

## Import Pipeline

```
File Selected
     │
     ▼
Detect Format (by extension)
     │
     ▼
Start Background Thread
     │
     ├──▶ Read File into Memory
     │
     ├──▶ Parse (STL/OBJ/3MF loader)
     │
     ├──▶ Compute Hash (xxHash)
     │
     └──▶ Signal Complete
            │
            ▼ (main thread)
     Check Duplicates
            │
            ▼
     Upload to GPU
            │
            ▼
     Generate Thumbnail
            │
            ▼
     Store in Database
            │
            ▼
     Update Library Panel
```

## Format Detection

```cpp
auto detectFormat(const std::filesystem::path& path) -> FileFormat {
    auto ext = path.extension().string();
    // Convert to lowercase
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == ".stl") return FileFormat::STL;
    if (ext == ".obj") return FileFormat::OBJ;
    if (ext == ".3mf") return FileFormat::ThreeMF;
    if (ext == ".gcode" || ext == ".nc" || ext == ".ngc") return FileFormat::GCode;

    return FileFormat::Unknown;
}
```

## Binary STL Detection

```cpp
auto isBinarySTL(const std::vector<uint8_t>& data) -> bool {
    if (data.size() < 84) return false;

    // Check if starts with "solid" (ASCII indicator)
    // But binary files can also start with "solid" in header
    // So also check expected size based on triangle count

    uint32_t triangleCount;
    std::memcpy(&triangleCount, &data[80], sizeof(uint32_t));

    size_t expectedSize = 84 + (triangleCount * 50);
    return data.size() == expectedSize;
}
```
