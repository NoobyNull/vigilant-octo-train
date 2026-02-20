# Phase 01: Add Materials Mapping to Object View with Coordinated Materials Database - Research

**Researched:** 2026-02-09
**Domain:** Materials database with ZIP-based archive format, OpenGL texture mapping, ImGui UI panels
**Confidence:** HIGH

## Summary

This phase introduces a wood species materials system for CNC woodworking applications, combining a SQLite-backed global materials database with portable .dwmat archive files (ZIP format), texture-mapped 3D visualization, and a dedicated materials browser panel. The implementation leverages existing infrastructure (ConnectionPool, EventBus, ImGui panels) while adding new subsystems for texture loading (stb_image), ZIP handling (miniz), JSON metadata (nlohmann/json), and OpenGL texture mapping with grain direction control.

**Key architectural insight:** The hybrid storage model (SQLite record + external .dwmat file) mirrors the existing model import pattern (database metadata + external model file), making this a natural extension of the current architecture. The .dwmat archive is the portable unit containing texture PNG + JSON specs, while SQLite provides fast querying and global library management.

**Primary recommendation:** Use miniz (single-file public domain) for ZIP handling, stb_image (public domain) for PNG loading, and nlohmann/json (MIT, header-only) for material specs. Implement planar/box projection for automatic UV coordinate generation with user-controllable grain direction rotation. Create MaterialRepository following the ModelRepository pattern, and MaterialsPanel following the LibraryPanel pattern with thumbnail preview grid.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

**Materials database:**
- Support wood species and composite woods (MDF, plywood, etc.)
- Categories: hardwood, softwood, exotic, domestic — with specific species under each
- User-editable with built-in defaults shipped with the app
- Storage: hybrid — SQLite entry in existing database pointing to a .dwmat material archive file
- Global library shared across all projects (not per-project)

**Material archive format (.dwmat):**
- ZIP-based archive with `.dwmat` extension
- Contains: texture PNG (tileable and/or photo), technical specifications (JSON or XML), metadata
- User provides the texture images — app reads them from the archive
- Import and export of .dwmat archives required (share materials between users/machines)

**Object view integration:**
- Material replaces the existing color selector as an overlay image on the object
- Wood species database gets its own dedicated panel (not inline in another panel)
- All fields in the material record must be editable in the panel
- Objects with no material assigned: blank/hidden (no material info shown)
- One model loaded at a time — material assignment is per-object, one at a time

**3D visualization:**
- Texture mapped: apply the wood species PNG texture to the 3D object surface
- Grain direction: auto-detected from texture orientation, but user can modify/override
- Grain direction affects texture mapping on the object

**Material properties:**
- Janka hardness rating
- Feed and speed recommendations (CNC-specific: feed rate, spindle speed, depth of cut)
- Cost per board-foot (for project cost estimation)
- Grain direction (auto + user-modifiable)
- Texture image(s) from the .dwmat archive

### Claude's Discretion

- Database schema design for the SQLite materials table
- Default built-in species list (common North American hardwoods/softwoods)
- Panel layout and widget arrangement for the materials browser
- Texture mapping UV approach (how grain direction maps to object geometry)
- Archive internal structure (file naming within the .dwmat ZIP)

### Deferred Ideas (OUT OF SCOPE)

None — discussion stayed within phase scope

</user_constraints>

## Standard Stack

### Core Dependencies

| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| **miniz** | 3.0.2+ | ZIP archive read/write for .dwmat files | Public domain, single-file (miniz.c/h), battle-tested, no external deps — standard for embedded ZIP handling in C/C++ |
| **stb_image** | 2.30+ | PNG texture loading | Public domain/MIT dual-license, single-header, industry-standard for game/graphics texture loading |
| **nlohmann/json** | 3.11+ | JSON parsing for material specs | MIT license, header-only, most popular modern C++ JSON library, already used in many C++17 projects |

### Supporting (Already in Project)

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| OpenGL 3.3 | Core | Texture mapping via sampler2D | Already project standard for all 3D rendering |
| ImGui | docking | Materials browser panel UI | Already project standard for all UI panels |
| SQLite 3.45+ | 3.45.0 | Materials database with foreign key to .dwmat path | Already project standard via ConnectionPool |
| ZLIB | 1.3.1 | Deflate compression (miniz can use zlib if available) | Already in project for 3MF support — miniz can work standalone or with zlib |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| miniz | libzip (BSD) | libzip is more feature-complete but adds dependency and build complexity — miniz is sufficient for simple ZIP read/write |
| stb_image | libpng (permissive) | libpng is more robust but requires external dependency — stb_image is simpler and sufficient for tileable textures |
| nlohmann/json | RapidJSON (MIT) | RapidJSON is faster but more complex API — nlohmann/json is easier to use and sufficient for material metadata |
| JSON | XML (pugixml) | User constraint specifies "JSON or XML" — JSON chosen for simplicity and ecosystem preference |

**Installation:**

Dependencies already use FetchContent pattern. Add to `cmake/Dependencies.cmake`:

```cmake
# miniz - ZIP archive support for .dwmat files
FetchContent_Declare(
    miniz
    GIT_REPOSITORY https://github.com/richgel999/miniz.git
    GIT_TAG 3.0.2
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(miniz)
add_library(miniz STATIC ${miniz_SOURCE_DIR}/miniz.c)
target_include_directories(miniz PUBLIC ${miniz_SOURCE_DIR})

# stb_image - PNG texture loading (header-only)
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG master
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(stb)
# Header-only — include stb_image.h with STB_IMAGE_IMPLEMENTATION in one .cpp

# nlohmann/json - JSON parsing for material specs (header-only)
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(json)
# Header-only — include nlohmann/json.hpp
```

## Architecture Patterns

### Recommended Project Structure

```
src/
├── core/
│   ├── database/
│   │   ├── material_repository.h/cpp    # MaterialRepository (mirrors ModelRepository)
│   │   └── schema.cpp                   # Add materials table to existing schema
│   ├── materials/
│   │   ├── material.h                   # MaterialRecord struct, MaterialCategory enum
│   │   ├── material_archive.h/cpp       # .dwmat ZIP read/write (mirrors ProjectArchive)
│   │   ├── material_loader.h/cpp        # Load texture + JSON from .dwmat
│   │   └── default_materials.h/cpp      # Built-in species definitions
│   └── loaders/
│       └── texture_loader.h/cpp         # PNG loading via stb_image
├── render/
│   ├── texture.h/cpp                    # Texture class (GLuint wrapper with RAII)
│   ├── renderer.h/cpp                   # Add texture binding support to existing renderer
│   └── shader_sources.h                 # Add textured mesh shader variant
└── ui/
    ├── panels/
    │   ├── materials_panel.h/cpp        # New: materials browser with thumbnail grid
    │   └── properties_panel.h/cpp       # Modify: replace color picker with material selector
    └── dialogs/
        └── material_editor_dialog.h/cpp # Edit material properties (Janka, feed/speed, cost)
```

### Pattern 1: Repository Pattern (MaterialRepository)

**What:** SQLite CRUD operations for material records, following established ModelRepository pattern

**When to use:** All database access for materials — create, read, update, delete, search by category/name

**Example (based on existing ModelRepository pattern):**

```cpp
// src/core/materials/material.h
struct MaterialRecord {
    i64 id = 0;
    std::string name;                    // "Red Oak", "Maple (Hard)", "Baltic Birch Plywood"
    MaterialCategory category;            // Hardwood, Softwood, Exotic, Composite
    Path archivePath;                     // Absolute path to .dwmat file
    float jankaHardness = 0.0f;          // lbf (pounds-force)
    float feedRate = 0.0f;               // inches/min
    float spindleSpeed = 0.0f;           // RPM
    float depthOfCut = 0.0f;             // inches
    float costPerBoardFoot = 0.0f;       // USD
    float grainDirectionDeg = 0.0f;      // 0-360 degrees, user-modifiable
    Path thumbnailPath;                   // Generated thumbnail of texture (TGA format)
    std::string importedAt;
};

// src/core/database/material_repository.h
class MaterialRepository {
public:
    explicit MaterialRepository(Database& db);

    std::optional<i64> insert(const MaterialRecord& material);
    std::optional<MaterialRecord> findById(i64 id);
    std::vector<MaterialRecord> findAll();
    std::vector<MaterialRecord> findByCategory(MaterialCategory category);
    std::vector<MaterialRecord> findByName(std::string_view searchTerm);
    bool update(const MaterialRecord& material);
    bool remove(i64 id);

private:
    Database& m_db;
};
```

### Pattern 2: Archive Pattern (.dwmat ZIP files)

**What:** ZIP-based material archive mirroring existing ProjectArchive pattern, containing texture PNG + metadata JSON

**When to use:** Import/export of .dwmat files, extracting texture for OpenGL loading

**Archive internal structure (Claude's discretion):**

```
material.dwmat (ZIP file)
├── texture.png          # Tileable wood grain texture
├── metadata.json        # Material properties (Janka, feed/speed, cost, grain)
└── thumbnail.png        # Optional: small preview (64x64 or 128x128)
```

**metadata.json format:**

```json
{
    "name": "Red Oak",
    "category": "hardwood",
    "janka_hardness": 1290.0,
    "feed_rate": 100.0,
    "spindle_speed": 18000.0,
    "depth_of_cut": 0.125,
    "cost_per_board_foot": 4.50,
    "grain_direction_deg": 0.0,
    "texture_file": "texture.png",
    "version": 1
}
```

**Example (based on existing ProjectArchive):**

```cpp
// src/core/materials/material_archive.h
class MaterialArchive {
public:
    // Create .dwmat from texture PNG + metadata
    static ArchiveResult create(const std::string& archivePath,
                                 const std::string& texturePath,
                                 const MaterialRecord& metadata);

    // Extract .dwmat to temp directory, load texture + metadata
    static std::optional<MaterialData> load(const std::string& archivePath);

    // List contents of .dwmat
    static std::vector<ArchiveEntry> list(const std::string& archivePath);

    static constexpr const char* Extension = ".dwmat";
};

struct MaterialData {
    std::vector<uint8_t> textureData;  // PNG bytes
    int textureWidth;
    int textureHeight;
    MaterialRecord metadata;
};
```

### Pattern 3: Texture Loading (stb_image)

**What:** Single-header PNG loader for wood grain textures

**When to use:** Loading texture.png from extracted .dwmat archive

**Example:**

```cpp
// src/core/loaders/texture_loader.cpp
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// src/core/loaders/texture_loader.h
class TextureLoader {
public:
    struct TextureData {
        std::vector<uint8_t> pixels;
        int width;
        int height;
        int channels;
    };

    static std::optional<TextureData> loadPNG(const Path& path);
    static std::optional<TextureData> loadPNGFromMemory(const std::vector<uint8_t>& data);
};

// Implementation
std::optional<TextureData> TextureLoader::loadPNG(const Path& path) {
    int width, height, channels;
    unsigned char* data = stbi_load(path.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (!data) {
        log::errorf("TextureLoader", "Failed to load PNG: %s", stbi_failure_reason());
        return std::nullopt;
    }

    TextureData result;
    result.width = width;
    result.height = height;
    result.channels = 4; // RGBA
    result.pixels.assign(data, data + (width * height * 4));
    stbi_image_free(data);

    return result;
}
```

### Pattern 4: OpenGL Texture Wrapper (RAII)

**What:** RAII wrapper for OpenGL texture ID, mirroring existing Shader/Framebuffer pattern

**When to use:** Managing texture lifecycle, preventing leaks

**Example:**

```cpp
// src/render/texture.h
class Texture {
public:
    Texture();
    ~Texture();

    // No copy, move allowed
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;

    // Upload texture data to GPU
    bool upload(const uint8_t* data, int width, int height, GLenum format = GL_RGBA);

    // Bind for rendering
    void bind(unsigned int slot = 0) const;
    void unbind() const;

    GLuint id() const { return m_id; }
    bool isValid() const { return m_id != 0; }

    // Set texture parameters
    void setWrap(GLenum wrap); // GL_REPEAT for tileable textures
    void setFilter(GLenum minFilter, GLenum magFilter);

private:
    GLuint m_id = 0;
};
```

### Pattern 5: UV Coordinate Generation (Planar/Box Projection)

**What:** Automatic UV coordinate generation for loaded meshes that lack UVs

**When to use:** When applying material to models imported without texture coordinates (STL files)

**Approach (Claude's discretion):**

- **Planar projection:** Project vertices onto dominant axis plane (XY, XZ, or YZ based on mesh bounds)
- **Box mapping:** Project onto 6 faces, select based on vertex normal direction
- **Grain direction:** Rotate UVs by user-specified angle (0-360 degrees) in texture space

**Example (planar projection for wood grain along longest axis):**

```cpp
// src/core/mesh/mesh.h
class Mesh {
public:
    // Generate UVs if not present (for texture mapping)
    void generatePlanarUVs(float rotation = 0.0f);
    void generateBoxUVs(float rotation = 0.0f);

    // Rotate existing UVs (for grain direction control)
    void rotateUVs(float degrees);
};

// Implementation
void Mesh::generatePlanarUVs(float rotation) {
    if (m_vertices.empty()) return;

    Bounds bounds = computeBounds();
    Vec3 size = bounds.max - bounds.min;

    // Choose dominant plane (longest 2 axes)
    // For wood grain: typically XZ (horizontal) or XY (vertical)
    bool useXZ = (size.x * size.z) > (size.x * size.y);

    for (auto& v : m_vertices) {
        if (useXZ) {
            v.texCoord.x = (v.position.x - bounds.min.x) / size.x;
            v.texCoord.y = (v.position.z - bounds.min.z) / size.z;
        } else {
            v.texCoord.x = (v.position.x - bounds.min.x) / size.x;
            v.texCoord.y = (v.position.y - bounds.min.y) / size.y;
        }

        // Apply grain direction rotation
        if (rotation != 0.0f) {
            float rad = glm::radians(rotation);
            float u = v.texCoord.x - 0.5f;
            float v_coord = v.texCoord.y - 0.5f;
            v.texCoord.x = u * cos(rad) - v_coord * sin(rad) + 0.5f;
            v.texCoord.y = u * sin(rad) + v_coord * cos(rad) + 0.5f;
        }
    }
}
```

### Pattern 6: Material Assignment (Object-Centric)

**What:** Store material ID per loaded object, render with texture instead of solid color

**When to use:** User selects material from MaterialsPanel, assigns to currently loaded object

**Example:**

```cpp
// src/ui/panels/properties_panel.h
class PropertiesPanel : public Panel {
public:
    // Replace color picker with material selector
    void setMaterialId(i64 materialId);
    i64 getMaterialId() const { return m_materialId; }

private:
    void renderMaterialInfo();  // Replaces renderMaterialInfo (old color picker)

    i64 m_materialId = -1;  // -1 = no material assigned
    std::optional<MaterialRecord> m_material;  // Cached from MaterialRepository
};

// Renderer integration
// src/render/renderer.h
class Renderer {
public:
    void renderMesh(const GPUMesh& gpuMesh,
                    const Texture* texture,  // nullptr = no texture (solid color fallback)
                    const Mat4& modelMatrix);
};
```

### Anti-Patterns to Avoid

- **Storing texture data in SQLite BLOBs:** Keep textures in .dwmat files, SQLite only stores path — prevents database bloat, enables easy sharing
- **Per-object material storage:** Materials are global library entries, objects reference by ID — prevents duplication, enables library management
- **Hardcoding material list:** Ship defaults via `default_materials.h` but allow full user editing — woodworkers have regional preferences
- **Ignoring UV generation:** STL files lack UVs, must generate on load — otherwise texture won't display
- **Tight coupling texture to color:** Material replaces color as primary surface appearance — color becomes fallback for no-material state

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| ZIP file handling | Custom binary archive format | miniz (public domain) | ZIP is standard, miniz handles compression/decompression edge cases, cross-platform compatibility |
| PNG decoding | Custom image parser | stb_image (public domain) | PNG format is complex (chunks, filters, interlacing) — stb_image handles all variants |
| JSON parsing | String manipulation parser | nlohmann/json (MIT) | JSON escaping, Unicode, nested objects are error-prone — library is battle-tested |
| OpenGL texture management | Raw glGenTextures calls | RAII Texture wrapper | Prevents leaks, follows existing Shader/Framebuffer pattern, exception-safe |
| UV coordinate generation | Ad-hoc vertex loops | Established planar/box projection algorithms | Edge cases (degenerate triangles, zero-area faces) need careful handling |

**Key insight:** All three core dependencies (miniz, stb_image, nlohmann/json) are permissive/public domain, single-file or header-only, and industry-standard for embedded use in C++ applications. Don't reinvent these wheels — focus effort on domain logic (material database, UI panels, CNC-specific properties).

## Common Pitfalls

### Pitfall 1: Missing UV Coordinates on STL Import

**What goes wrong:** STL format has no texture coordinate spec, imported meshes have all UVs = (0,0), texture appears as single-color smear

**Why it happens:** STL is purely geometric (vertices + normals), texture mapping was not part of original format design

**How to avoid:** Generate UVs on mesh load if absent — detect by checking if all vertices have texCoord == Vec2(0,0), then call generatePlanarUVs()

**Warning signs:** Material texture appears as solid color or heavily distorted, even though PNG loaded correctly

**Prevention strategy:**

```cpp
// In mesh loader (STL, OBJ, 3MF)
if (needsUVGeneration(mesh)) {
    mesh.generatePlanarUVs();  // Auto-generate before saving to database
}

bool needsUVGeneration(const Mesh& mesh) {
    // Check if all UVs are zero (STL default)
    return std::all_of(mesh.vertices().begin(), mesh.vertices().end(),
                       [](const Vertex& v) { return v.texCoord.x == 0.0f && v.texCoord.y == 0.0f; });
}
```

### Pitfall 2: Texture Seams at UV Boundaries

**What goes wrong:** Visible seams/lines where texture wraps around at UV boundaries (0→1 edge)

**Why it happens:** Non-tileable textures or incorrect wrap mode (GL_CLAMP_TO_EDGE instead of GL_REPEAT)

**How to avoid:**
1. Require tileable textures in .dwmat (user responsibility)
2. Set GL_REPEAT wrap mode for material textures
3. Use mipmaps to reduce aliasing at distance

**Warning signs:** Visible horizontal/vertical lines on textured model, especially at 0/1 UV boundaries

**Prevention strategy:**

```cpp
// When uploading wood grain texture
texture.setWrap(GL_REPEAT);  // NOT GL_CLAMP_TO_EDGE
texture.setFilter(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);  // Trilinear filtering
glGenerateMipmap(GL_TEXTURE_2D);  // Generate mipmaps for distance LOD
```

### Pitfall 3: Archive Path Invalidation After Material Import

**What goes wrong:** Material imported from user's Downloads folder, user deletes original .dwmat, texture loading fails

**Why it happens:** Storing absolute path to user-provided file instead of copying to app-managed location

**How to avoid:** Copy imported .dwmat to application data directory (e.g., `~/.local/share/DigitalWorkshop/materials/`), store path relative to app data root

**Warning signs:** Materials load correctly after import but fail after restart or after user cleans up downloads

**Prevention strategy:**

```cpp
// On material import
Path appMaterialsDir = AppPaths::userDataDir() / "materials";
file::createDirectories(appMaterialsDir);

std::string archiveName = file::getFileName(userProvidedPath);
Path targetPath = appMaterialsDir / archiveName;

// Copy to managed location
file::copy(userProvidedPath, targetPath);

// Store managed path in database (not user's original path)
material.archivePath = targetPath;
```

### Pitfall 4: Shader Uniform Location Cache Invalidation

**What goes wrong:** Adding texture sampling to shader invalidates existing uniform location cache, rendering fails silently

**Why it happens:** Existing `Shader` class caches uniform locations, adding new uniforms (uTexture, uUseTexture) changes location indices

**How to avoid:** Extend shader to lazily cache new uniforms, ensure backward compatibility with non-textured rendering

**Warning signs:** Models render as black or previous color after texture shader added, no OpenGL errors

**Prevention strategy:**

```cpp
// Add textured variant of mesh shader, keep existing solid-color shader
// shader_sources.h
constexpr const char* MESH_TEXTURED_FRAGMENT = R"(
#version 330 core

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vTexCoord;

uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform vec3 uAmbient;
uniform vec3 uObjectColor;
uniform vec3 uViewPos;
uniform float uShininess;
uniform sampler2D uTexture;
uniform bool uUseTexture;  // Flag to toggle texture vs solid color

out vec4 FragColor;

void main() {
    vec3 normal = normalize(vNormal);
    vec3 lightDir = normalize(-uLightDir);

    // Base color: texture sample if enabled, else solid color
    vec3 baseColor = uUseTexture ? texture(uTexture, vTexCoord).rgb : uObjectColor;

    // Phong lighting (same as existing shader)
    // ...
}
)";
```

### Pitfall 5: Race Condition in Thumbnail Generation

**What goes wrong:** Generating material thumbnails (small texture previews) in parallel causes OpenGL context issues

**Why it happens:** OpenGL is not thread-safe, thumbnail generation via offscreen render must run on main thread

**How to avoid:** Follow existing pattern from ThumbnailGenerator — generate on main thread, or use MainThreadQueue for worker-initiated thumbnails

**Warning signs:** Intermittent crashes or black thumbnails when importing multiple materials, works fine for single import

**Prevention strategy:**

```cpp
// Reuse existing ThumbnailGenerator pattern for material texture thumbnails
// Generate 128x128 preview of wood grain texture
auto thumbnail = ThumbnailGenerator::generate(materialTexture, 128, 128);
file::writeBinary(thumbnailPath, thumbnail);  // TGA format

// If generating from worker thread (import pipeline):
m_mainThreadQueue.enqueue([this, materialId, texturePath]() {
    auto thumbnail = ThumbnailGenerator::generate(texturePath, 128, 128);
    m_materialRepo->updateThumbnail(materialId, thumbnail);
});
```

## Code Examples

Verified patterns from official sources and existing codebase:

### Example 1: ZIP Archive Read/Write (miniz)

```cpp
// Based on: https://github.com/richgel999/miniz
// Creating .dwmat archive

#include <miniz.h>

bool MaterialArchive::create(const std::string& archivePath,
                             const std::string& texturePath,
                             const MaterialRecord& metadata) {
    mz_zip_archive zip;
    memset(&zip, 0, sizeof(zip));

    if (!mz_zip_writer_init_file(&zip, archivePath.c_str(), 0)) {
        return false;
    }

    // Add texture PNG
    auto textureData = file::readBinary(texturePath);
    if (!textureData) {
        mz_zip_writer_end(&zip);
        return false;
    }
    mz_zip_writer_add_mem(&zip, "texture.png", textureData->data(),
                          textureData->size(), MZ_DEFAULT_COMPRESSION);

    // Add metadata JSON
    nlohmann::json j = metadataToJson(metadata);
    std::string jsonStr = j.dump(4);  // Pretty-print with 4-space indent
    mz_zip_writer_add_mem(&zip, "metadata.json", jsonStr.data(),
                          jsonStr.size(), MZ_DEFAULT_COMPRESSION);

    mz_zip_writer_finalize_archive(&zip);
    mz_zip_writer_end(&zip);

    return true;
}

// Extracting .dwmat archive
std::optional<MaterialData> MaterialArchive::load(const std::string& archivePath) {
    mz_zip_archive zip;
    memset(&zip, 0, sizeof(zip));

    if (!mz_zip_reader_init_file(&zip, archivePath.c_str(), 0)) {
        return std::nullopt;
    }

    MaterialData result;

    // Extract texture PNG to memory
    size_t texSize;
    void* texData = mz_zip_reader_extract_file_to_heap(&zip, "texture.png", &texSize, 0);
    if (texData) {
        result.textureData.assign((uint8_t*)texData, (uint8_t*)texData + texSize);
        mz_free(texData);
    }

    // Extract metadata JSON
    size_t jsonSize;
    void* jsonData = mz_zip_reader_extract_file_to_heap(&zip, "metadata.json", &jsonSize, 0);
    if (jsonData) {
        std::string jsonStr((char*)jsonData, jsonSize);
        auto j = nlohmann::json::parse(jsonStr);
        result.metadata = jsonToMaterial(j);
        mz_free(jsonData);
    }

    mz_zip_reader_end(&zip);
    return result;
}
```

### Example 2: Textured Mesh Rendering (OpenGL 3.3)

```cpp
// Based on: https://learnopengl.com/Getting-started/Textures
// Shader setup and texture binding

// Vertex shader (already in shader_sources.h)
// layout (location = 2) in vec2 aTexCoord;  // Already present
// out vec2 vTexCoord;

// Fragment shader addition
uniform sampler2D uMaterialTexture;
uniform bool uHasMaterial;  // true if material assigned, false = use solid color

void main() {
    vec3 baseColor;
    if (uHasMaterial) {
        baseColor = texture(uMaterialTexture, vTexCoord).rgb;
    } else {
        baseColor = uObjectColor;  // Fallback to solid color
    }

    // Apply Phong lighting to baseColor (existing code)
    // ...
}

// C++ rendering code
void Renderer::renderMesh(const GPUMesh& gpuMesh, const Texture* material,
                          const Mat4& modelMatrix) {
    m_meshShader.use();

    // Bind material texture if present
    if (material && material->isValid()) {
        m_meshShader.setInt("uHasMaterial", 1);
        material->bind(0);  // Texture unit 0
        m_meshShader.setInt("uMaterialTexture", 0);
    } else {
        m_meshShader.setInt("uHasMaterial", 0);
        // Use existing uObjectColor uniform for solid color
    }

    // Set matrices and lighting (existing code)
    m_meshShader.setMat4("uModel", modelMatrix);
    // ...

    // Draw
    glBindVertexArray(gpuMesh.vao);
    glDrawElements(GL_TRIANGLES, gpuMesh.indexCount, GL_UNSIGNED_INT, 0);
}
```

### Example 3: ImGui Materials Browser Panel

```cpp
// Based on: existing LibraryPanel pattern
// Materials browser with thumbnail grid and category filtering

void MaterialsPanel::render() {
    if (!m_open) return;

    if (ImGui::Begin("Materials", &m_open)) {
        renderToolbar();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Category filter tabs
        if (ImGui::BeginTabBar("MaterialCategories")) {
            if (ImGui::BeginTabItem("All")) {
                renderMaterialGrid(m_allMaterials);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Hardwood")) {
                renderMaterialGrid(filterByCategory(MaterialCategory::Hardwood));
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Softwood")) {
                renderMaterialGrid(filterByCategory(MaterialCategory::Softwood));
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Exotic")) {
                renderMaterialGrid(filterByCategory(MaterialCategory::Exotic));
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Composite")) {
                renderMaterialGrid(filterByCategory(MaterialCategory::Composite));
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}

void MaterialsPanel::renderMaterialGrid(const std::vector<MaterialRecord>& materials) {
    float thumbnailSize = 96.0f;
    float cellSize = thumbnailSize + 16.0f;
    int columns = std::max(1, (int)(ImGui::GetContentRegionAvail().x / cellSize));

    for (size_t i = 0; i < materials.size(); ++i) {
        const auto& mat = materials[i];

        ImGui::BeginGroup();

        // Thumbnail (reuse existing texture cache pattern from LibraryPanel)
        GLuint texID = getThumbnailTexture(mat);
        if (texID != 0) {
            ImGui::Image((void*)(intptr_t)texID, ImVec2(thumbnailSize, thumbnailSize));
        } else {
            ImGui::Dummy(ImVec2(thumbnailSize, thumbnailSize));
        }

        // Material name
        ImGui::TextWrapped("%s", mat.name.c_str());

        // Click to select
        if (ImGui::IsItemClicked()) {
            m_selectedMaterialId = mat.id;
            if (m_onMaterialSelected) {
                m_onMaterialSelected(mat.id);
            }
        }

        // Double-click to assign to current object
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
            if (m_onMaterialAssigned) {
                m_onMaterialAssigned(mat.id);
            }
        }

        ImGui::EndGroup();

        // Grid layout
        if ((i + 1) % columns != 0 && i + 1 < materials.size()) {
            ImGui::SameLine();
        }
    }
}
```

### Example 4: JSON Metadata Serialization

```cpp
// Based on: https://github.com/nlohmann/json
// Material metadata JSON read/write

#include <nlohmann/json.hpp>
using json = nlohmann::json;

// Serialize MaterialRecord to JSON
json MaterialArchive::metadataToJson(const MaterialRecord& mat) {
    return {
        {"name", mat.name},
        {"category", materialCategoryToString(mat.category)},
        {"janka_hardness", mat.jankaHardness},
        {"feed_rate", mat.feedRate},
        {"spindle_speed", mat.spindleSpeed},
        {"depth_of_cut", mat.depthOfCut},
        {"cost_per_board_foot", mat.costPerBoardFoot},
        {"grain_direction_deg", mat.grainDirectionDeg},
        {"texture_file", "texture.png"},
        {"version", 1}
    };
}

// Deserialize JSON to MaterialRecord
MaterialRecord MaterialArchive::jsonToMaterial(const json& j) {
    MaterialRecord mat;
    mat.name = j.value("name", "");
    mat.category = stringToMaterialCategory(j.value("category", "hardwood"));
    mat.jankaHardness = j.value("janka_hardness", 0.0f);
    mat.feedRate = j.value("feed_rate", 0.0f);
    mat.spindleSpeed = j.value("spindle_speed", 0.0f);
    mat.depthOfCut = j.value("depth_of_cut", 0.0f);
    mat.costPerBoardFoot = j.value("cost_per_board_foot", 0.0f);
    mat.grainDirectionDeg = j.value("grain_direction_deg", 0.0f);
    return mat;
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Solid color only | Texture-mapped materials | Industry standard (2000s+) | Realistic wood grain visualization for CNC planning |
| Hardcoded material properties | User-editable material database | CAM software trend (2010s+) | Customization for regional wood availability, supplier pricing |
| Color picker in properties panel | Dedicated materials browser | Modern CAD/CAM UX (2015+) | Clearer separation of appearance (material) vs geometry (properties) |
| XML for metadata | JSON for metadata | Ecosystem shift (~2015) | Better C++ library support, simpler parsing, web compatibility |
| Custom archive formats | ZIP-based (.dwmat) | Industry standard | Cross-platform compatibility, user can inspect with standard tools |

**Deprecated/outdated:**

- **Pre-generated UVs only:** Modern approach auto-generates UVs for formats lacking them (STL) — prevents "texture won't show" frustration
- **BLOB storage in database:** Keep large binary data (textures) in files, database stores paths — prevents database bloat, enables caching
- **Fixed material list:** User-editable with defaults shipped — accommodates regional wood species variations
- **Per-project materials:** Global material library shared across projects — reduces duplication, enables reuse

## Open Questions

### Question 1: Default Material Species List

**What we know:** User constraint requires "built-in defaults shipped with the app" for common North American hardwoods/softwoods

**What's unclear:** Exact species list (quantity and specific names), whether to include international species in defaults

**Recommendation:** Ship 20-30 common species covering:
- **Hardwoods (10-12):** Red Oak, White Oak, Maple (Hard), Maple (Soft), Cherry, Walnut, Ash, Birch (Yellow), Hickory, Mahogany, Poplar
- **Softwoods (6-8):** Pine (White), Pine (Yellow), Douglas Fir, Cedar (Western Red), Spruce, Hemlock, Redwood
- **Composites (4-6):** MDF, Plywood (Baltic Birch), Plywood (Hardwood), Particle Board, OSB

Include Janka ratings from authoritative source ([The Wood Database](https://www.wood-database.com/wood-articles/janka-hardness/)), conservative feed/speed defaults based on CNC industry guides ([CNC Cookbook](https://www.cnccookbook.com/feeds-speeds-cnc-wood-cutting/)). Users can edit or add regional species.

### Question 2: Grain Direction Auto-Detection

**What we know:** User constraint specifies "grain direction: auto-detected from texture orientation, but user can modify/override"

**What's unclear:** Algorithm for auto-detecting grain direction from texture image (frequency analysis? user hint in metadata?)

**Recommendation:** Default to 0 degrees (horizontal grain), rely on user to set correct direction when creating .dwmat. Consider future enhancement: Fourier transform to detect dominant grain orientation, but defer as out-of-scope for Phase 01. Store grain direction in metadata.json, allow rotation in MaterialsPanel or PropertiesPanel.

### Question 3: Material Assignment Persistence

**What we know:** Material assignment is per-object, one model loaded at a time

**What's unclear:** Where to persist material assignment (database? in-memory only? per-project?)

**Recommendation:** Store material ID in models table (add `material_id` column with foreign key to materials table). This enables:
- Material persists across app restarts
- Material info shown in library preview (before full load)
- Per-project material usage tracking (future feature)

Schema addition:
```sql
ALTER TABLE models ADD COLUMN material_id INTEGER DEFAULT NULL;
ALTER TABLE models ADD FOREIGN KEY (material_id) REFERENCES materials(id) ON DELETE SET NULL;
```

## Sources

### Primary (HIGH confidence)

**Existing Codebase Analysis:**
- `/data/DW/src/core/database/schema.cpp` - Database schema pattern (models, projects, cost_estimates tables)
- `/data/DW/src/core/archive/archive.cpp` - Custom archive implementation pattern for .dwp files
- `/data/DW/src/render/shader_sources.h` - Existing shader infrastructure with texture coordinate support
- `/data/DW/src/core/mesh/vertex.h` - Vertex struct already includes texCoord field
- `/data/DW/cmake/Dependencies.cmake` - FetchContent dependency management pattern

**Official Documentation:**
- [miniz GitHub Repository](https://github.com/richgel999/miniz) - Public domain ZIP library, single-file
- [stb_image GitHub Repository](https://github.com/nothings/stb/blob/master/stb_image.h) - Public domain PNG loader, v2.30 (2024-05-31)
- [nlohmann/json GitHub Repository](https://github.com/nlohmann/json) - MIT license JSON library, header-only, v3.11+
- [LearnOpenGL - Textures](https://learnopengl.com/Getting-started/Textures) - OpenGL texture mapping fundamentals, UV coordinates

### Secondary (MEDIUM confidence)

**Wood Species and CNC Data:**
- [The Wood Database - Janka Hardness](https://www.wood-database.com/wood-articles/janka-hardness/) - 500+ species with Janka ratings
- [CNC Cookbook - Wood Feeds and Speeds](https://www.cnccookbook.com/feeds-speeds-cnc-wood-cutting/) - Feed rate calculation by Janka hardness
- [Bell Forest Products - Janka Hardness Chart](https://www.bellforestproducts.com/info/janka-hardness/) - Common North American species reference

**UV Coordinate Generation:**
- [Texture Mapping - Ray Tracer Challenge](http://raytracerchallenge.com/bonus/texture-mapping.html) - Planar projection algorithm explanation
- [Biplanar Mapping - Inigo Quilez](https://iquilezles.org/articles/biplanar/) - Advanced triplanar/biplanar mapping techniques
- [OpenGL Texture Coordinate Generation](https://www.informit.com/articles/article.aspx?p=770639&seqNum=4) - GL_OBJECT_LINEAR and planar mapping

**ImGui Patterns:**
- [ImGui Color Picker GitHub Issue #346](https://github.com/ocornut/imgui/issues/346) - ColorEdit/ColorPicker widget discussion
- [ImGuiFileDialog GitHub Repository](https://github.com/aiekick/ImGuiFileDialog) - File browser pattern for import/export dialogs

### Tertiary (LOW confidence)

- [ImGui Material Selector Panel Design Pattern] - No specific 2026 sources found; rely on existing LibraryPanel pattern from codebase
- [Wood Grain Direction Auto-Detection] - No established algorithm found; recommend manual user input for Phase 01

## Metadata

**Confidence breakdown:**
- Standard stack: **HIGH** - All dependencies verified from official GitHub repos, permissive licenses confirmed, widely used in C++ graphics applications
- Architecture: **HIGH** - Patterns mirror existing codebase (Repository, Archive, Panel), texture mapping is standard OpenGL 3.3
- Pitfalls: **MEDIUM** - UV generation and texture seams are known issues in graphics programming, specific to this use case but well-documented in community resources
- Default materials list: **MEDIUM** - Janka ratings verified from Wood Database, feed/speed values need validation with CNC experts
- Grain direction auto-detection: **LOW** - No established algorithm found, recommend manual user input

**Research date:** 2026-02-09
**Valid until:** 2026-03-09 (30 days - stable technologies, infrequent updates)

**Notes:**
- All three core dependencies (miniz, stb_image, nlohmann/json) are stable, mature libraries with infrequent breaking changes
- OpenGL 3.3 texture mapping is a stable API (2010 spec)
- ImGui docking branch is project standard, no migration concerns
- Wood species data and CNC parameters may need periodic updates as user adds materials, but schema is extensible
