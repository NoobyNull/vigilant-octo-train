#include "tag_image_dialog.h"

#include <cstring>
#include <sstream>

#include <imgui.h>

namespace dw {

TagImageDialog::TagImageDialog() : Dialog("Tag Image") {}

void TagImageDialog::open(const ModelRecord& record, GLuint thumbnailTexture) {
    m_record = record;
    m_thumbnailTexture = thumbnailTexture;
    m_error.clear();

    // Clear buffers
    std::memset(m_title, 0, sizeof(m_title));
    std::memset(m_description, 0, sizeof(m_description));
    std::memset(m_hover, 0, sizeof(m_hover));
    std::memset(m_keywords, 0, sizeof(m_keywords));
    std::memset(m_associations, 0, sizeof(m_associations));
    std::memset(m_categories, 0, sizeof(m_categories));

    m_state = State::Loading;
    m_open = true;

    // Fire the request callback
    if (m_onRequest) {
        m_onRequest(m_record.id);
    }
}

void TagImageDialog::setResult(const DescriptorResult& result) {
    if (!result.success) {
        m_error = result.error.empty() ? "Classification failed" : result.error;
        m_state = State::Ready;
        return;
    }

    m_error.clear();

    // Populate buffers from result
    std::strncpy(m_title, result.title.c_str(), sizeof(m_title) - 1);
    std::strncpy(m_description, result.description.c_str(), sizeof(m_description) - 1);
    std::strncpy(m_hover, result.hoverNarrative.c_str(), sizeof(m_hover) - 1);

    // Join vectors as comma-separated
    auto join = [](const std::vector<std::string>& v) -> std::string {
        std::string out;
        for (size_t i = 0; i < v.size(); ++i) {
            if (i > 0) out += ", ";
            out += v[i];
        }
        return out;
    };

    std::strncpy(m_keywords, join(result.keywords).c_str(), sizeof(m_keywords) - 1);
    std::strncpy(m_associations, join(result.associations).c_str(), sizeof(m_associations) - 1);
    std::strncpy(m_categories, join(result.categories).c_str(), sizeof(m_categories) - 1);

    m_state = State::Ready;
}

DescriptorResult TagImageDialog::buildResult() const {
    DescriptorResult r;
    r.success = true;
    r.title = m_title;
    r.description = m_description;
    r.hoverNarrative = m_hover;

    // Split comma-separated strings into vectors
    auto split = [](const char* buf) -> std::vector<std::string> {
        std::vector<std::string> out;
        std::istringstream ss(buf);
        std::string token;
        while (std::getline(ss, token, ',')) {
            // Trim whitespace
            size_t start = token.find_first_not_of(" \t");
            size_t end = token.find_last_not_of(" \t");
            if (start != std::string::npos && end != std::string::npos) {
                out.push_back(token.substr(start, end - start + 1));
            }
        }
        return out;
    };

    r.keywords = split(m_keywords);
    r.associations = split(m_associations);
    r.categories = split(m_categories);
    return r;
}

void TagImageDialog::render() {
    if (!m_open) {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(600, 420), ImGuiCond_FirstUseEver);
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (!ImGui::Begin("Tag Image", &m_open, ImGuiWindowFlags_NoDocking)) {
        ImGui::End();
        if (!m_open) {
            m_state = State::Idle;
        }
        return;
    }

    bool isLoading = (m_state == State::Loading);
    bool isReady = (m_state == State::Ready);

    // --- Layout: thumbnail on left, fields on right ---
    float thumbSize = 192.0f;
    ImVec2 thumbUV0(0, 0);
    ImVec2 thumbUV1(1, 1);

    ImGui::BeginGroup();
    // Model name header
    ImGui::TextUnformatted(m_record.name.c_str());
    ImGui::Spacing();

    if (m_thumbnailTexture != 0) {
        ImGui::Image(static_cast<ImTextureID>(m_thumbnailTexture),
                     ImVec2(thumbSize, thumbSize), thumbUV0, thumbUV1);
    } else {
        ImGui::Dummy(ImVec2(thumbSize, thumbSize));
        ImGui::SetCursorPos(ImGui::GetCursorPos()); // no-op, placeholder
    }
    ImGui::EndGroup();

    ImGui::SameLine();

    // --- Right side: fields ---
    ImGui::BeginGroup();

    if (isLoading) {
        ImGui::TextDisabled("Classifying...");
        ImGui::Spacing();
    }

    if (!m_error.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
        ImGui::TextWrapped("Error: %s", m_error.c_str());
        ImGui::PopStyleColor();
        ImGui::Spacing();
    }

    // Disable fields while loading
    if (isLoading) {
        ImGui::BeginDisabled();
    }

    float fieldWidth = ImGui::GetContentRegionAvail().x;

    ImGui::SetNextItemWidth(fieldWidth);
    ImGui::InputText("Title", m_title, sizeof(m_title));

    ImGui::SetNextItemWidth(fieldWidth);
    ImGui::InputTextMultiline("Description", m_description, sizeof(m_description),
                              ImVec2(fieldWidth, 60));

    ImGui::SetNextItemWidth(fieldWidth);
    ImGui::InputText("Hover", m_hover, sizeof(m_hover));

    ImGui::Spacing();

    ImGui::SetNextItemWidth(fieldWidth);
    ImGui::InputText("Keywords", m_keywords, sizeof(m_keywords));

    ImGui::SetNextItemWidth(fieldWidth);
    ImGui::InputText("Associations", m_associations, sizeof(m_associations));

    ImGui::SetNextItemWidth(fieldWidth);
    ImGui::InputText("Categories", m_categories, sizeof(m_categories));

    if (isLoading) {
        ImGui::EndDisabled();
    }

    ImGui::EndGroup();

    // --- Buttons ---
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (isReady) {
        if (ImGui::Button("Save", ImVec2(100, 0))) {
            if (m_onSave) {
                m_onSave(m_record.id, buildResult());
            }
            m_open = false;
            m_state = State::Idle;
        }
        ImGui::SameLine();
    }

    if (ImGui::Button("Cancel", ImVec2(100, 0))) {
        m_open = false;
        m_state = State::Idle;
    }

    ImGui::End();
}

} // namespace dw
