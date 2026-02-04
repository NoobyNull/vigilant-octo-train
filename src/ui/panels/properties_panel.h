#pragma once

#include "../../core/mesh/mesh.h"
#include "panel.h"

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

private:
    void renderMeshInfo();
    void renderBoundsInfo();
    void renderTransformInfo();
    void renderMaterialInfo();

    std::shared_ptr<Mesh> m_mesh;
    std::string m_meshName;
};

}  // namespace dw
