#pragma once

#include "../mesh/vertex.h"
#include "../types.h"
#include "heightmap.h"
#include "model_fitter.h"

#include <atomic>
#include <future>
#include <string>
#include <vector>

namespace dw {
namespace carve {

enum class CarveJobState {
    Idle,
    Computing,
    Ready,
    Error
};

class CarveJob {
public:
    CarveJob() = default;
    ~CarveJob();

    // Non-copyable
    CarveJob(const CarveJob&) = delete;
    CarveJob& operator=(const CarveJob&) = delete;

    // Start heightmap generation (non-blocking)
    void startHeightmap(const std::vector<Vertex>& vertices,
                        const std::vector<u32>& indices,
                        const ModelFitter& fitter,
                        const FitParams& fitParams,
                        const HeightmapConfig& hmConfig);

    // Poll state (call from main thread)
    CarveJobState state() const;
    f32 progress() const;           // [0.0, 1.0]
    const Heightmap& heightmap() const;
    std::string errorMessage() const;

    // Cancel in-progress computation
    void cancel();

private:
    std::atomic<CarveJobState> m_state{CarveJobState::Idle};
    std::atomic<f32> m_progress{0.0f};
    std::atomic<bool> m_cancelled{false};
    Heightmap m_heightmap;
    std::string m_error;
    std::future<void> m_future;
};

} // namespace carve
} // namespace dw
