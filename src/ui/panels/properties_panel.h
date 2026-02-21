#pragma once

#include <functional>
#include <memory>

#include "../../core/database/model_repository.h"
#include "../../core/materials/material.h"
#include "../../core/mesh/mesh.h"
#include "../../core/types.h"
#include "panel.h"

namespace dw {

// Properties panel for displaying selected model information
class PropertiesPanel : public Panel {
  public:
    PropertiesPanel();
    ~PropertiesPanel() override = default;

    void render() override;

    // Set the mesh to display properties for (full 3D load)
    void setMesh(std::shared_ptr<Mesh> mesh, const std::string& name = "");

    // Set model record for metadata-only preview (no mesh loaded)
    void setModelRecord(const ModelRecord& record);

    void clearMesh();

    // Callback when mesh geometry is modified (transform, center, normalize)
    // The application should re-upload the mesh to the GPU when this fires.
    using MeshModifiedCallback = std::function<void()>;
    void setOnMeshModified(MeshModifiedCallback callback) {
        m_onMeshModified = std::move(callback);
    }

    // Callback when the object color changes
    using ColorChangedCallback = std::function<void(const Color&)>;
    void setOnColorChanged(ColorChangedCallback callback) {
        m_onColorChanged = std::move(callback);
    }

    // Access current object color
    const Color& objectColor() const { return m_objectColor; }

    // Material display and management
    void setMaterial(const MaterialRecord& material) { m_material = material; }
    void clearMaterial() { m_material.reset(); }

    // Callback when grain direction slider changes (0-360 degrees)
    using GrainDirectionCallback = std::function<void(float)>;
    void setOnGrainDirectionChanged(GrainDirectionCallback callback) {
        m_onGrainDirectionChanged = std::move(callback);
    }

    // Callback when user clicks "Remove Material"
    using MaterialRemovedCallback = std::function<void()>;
    void setOnMaterialRemoved(MaterialRemovedCallback callback) {
        m_onMaterialRemoved = std::move(callback);
    }

  private:
    void renderModelRecordInfo();
    void renderMeshInfo();
    void renderBoundsInfo();
    void renderTransformInfo();
    void renderMaterialInfo();

    std::shared_ptr<Mesh> m_mesh;
    std::string m_meshName;

    // Metadata-only preview (no mesh loaded)
    std::optional<ModelRecord> m_record;

    // Material color (local storage, wired to renderer via callback)
    Color m_objectColor = Color::fromHex(0x6699CC);

    // Assigned material (optional - null if no material assigned)
    std::optional<MaterialRecord> m_material;

    // Transform UI state
    float m_targetSize = 1.0f;
    float m_translate[3] = {0.0f, 0.0f, 0.0f};
    float m_rotateDeg[3] = {0.0f, 0.0f, 0.0f};
    float m_scaleVal[3] = {1.0f, 1.0f, 1.0f};

    // Callbacks
    MeshModifiedCallback m_onMeshModified;
    ColorChangedCallback m_onColorChanged;
    GrainDirectionCallback m_onGrainDirectionChanged;
    MaterialRemovedCallback m_onMaterialRemoved;
};

} // namespace dw
