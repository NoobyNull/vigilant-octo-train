#pragma once

#include "../../core/mesh/mesh.h"
#include "../../core/types.h"
#include "panel.h"

#include <functional>
#include <memory>

namespace dw {

// Properties panel for displaying selected model information
class PropertiesPanel : public Panel {
public:
    PropertiesPanel();
    ~PropertiesPanel() override = default;

    void render() override;

    // Set the mesh to display properties for
    void setMesh(std::shared_ptr<Mesh> mesh, const std::string& name = "");
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

private:
    void renderMeshInfo();
    void renderBoundsInfo();
    void renderTransformInfo();
    void renderMaterialInfo();

    std::shared_ptr<Mesh> m_mesh;
    std::string m_meshName;

    // Material color (local storage, wired to renderer via callback)
    Color m_objectColor = Color::fromHex(0x6699CC);

    // Callbacks
    MeshModifiedCallback m_onMeshModified;
    ColorChangedCallback m_onColorChanged;
};

}  // namespace dw
