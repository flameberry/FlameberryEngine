#include "UI.h"

#include <imgui.h>
#include <imgui/imgui_internal.h>

#include "Flameberry.h"

namespace Flameberry {

    const ImVec2& operator+(const ImVec2& a, const ImVec2& b) {
        return ImVec2{ a.x + b.x, a.y + b.y };
    }

    bool UI::Splitter(bool split_vertically, float thickness, float* size1, float* size2, float min_size1, float min_size2, float splitter_long_axis_size)
    {
        using namespace ImGui;
        ImGuiContext& g = *GImGui;
        ImGuiWindow* window = g.CurrentWindow;
        ImGuiID id = window->GetID("##Splitter");
        ImRect bb;
        bb.Min = window->DC.CursorPos + (split_vertically ? ImVec2(*size1, 0.0f) : ImVec2(0.0f, *size1));
        bb.Max = bb.Min + CalcItemSize(split_vertically ? ImVec2(thickness, splitter_long_axis_size) : ImVec2(splitter_long_axis_size, thickness), 0.0f, 0.0f);
        return SplitterBehavior(bb, id, split_vertically ? ImGuiAxis_X : ImGuiAxis_Y, size1, size2, min_size1, min_size2, 0.0f, 0.0f, 0xFF000000);
    }

    bool UI::CenteredButton(const char* label, float alignment)
    {
        ImGuiStyle& style = ImGui::GetStyle();

        float size = ImGui::CalcTextSize(label).x + style.FramePadding.x * 2.0f;
        float avail = ImGui::GetContentRegionAvail().x;

        float off = (avail - size) * alignment;
        if (off > 0.0f)
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);

        return ImGui::Button(label);
    }

    void UI::SearchBar(const char* label, const float width, char* inputBuffer, const uint32_t inputLength, const char* inputHint)
    {
        ImGui::PushItemWidth(width);
        ImGui::InputTextWithHint(label, inputHint, inputBuffer, inputLength);
        ImGui::PopItemWidth();
    }

    ImVec2 UI::ContentBrowserItem(const std::filesystem::path& filepath, float width, float height, ImTextureID icon)
    {
        bool isDirectory = std::filesystem::is_directory(filepath);

        ImGuiStyle& style = ImGui::GetStyle();

        const auto& framePadding = style.FramePadding;
        height += 2 * framePadding.y;

        const float textHeight = ImGui::GetTextLineHeightWithSpacing();
        const float fullWidth = width + 3.0f * framePadding.x;
        const float fullHeight = height + 2 * textHeight;

        if (!isDirectory)
        {
            const auto& cursorPos = ImGui::GetCursorScreenPos();
            ImGui::GetWindowDrawList()->AddRectFilled(cursorPos, ImVec2(cursorPos.x + fullWidth, cursorPos.y + height), 0xff151515, 3, ImDrawFlags_RoundCornersTopLeft | ImDrawFlags_RoundCornersTopRight);
            ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(cursorPos.x, cursorPos.y + height), ImVec2(cursorPos.x + fullWidth, cursorPos.y + fullHeight), 0xff353535, 3, ImDrawFlags_RoundCornersBottomLeft | ImDrawFlags_RoundCornersBottomRight);
            ImGui::GetWindowDrawList()->AddRect(cursorPos, ImVec2(cursorPos.x + fullWidth, cursorPos.y + fullHeight), 0xff000000, 3, 0, 1.5f);
        }

        ImGui::BeginGroup();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));

        ImGui::ImageButton(filepath.c_str(), reinterpret_cast<ImTextureID>(icon), ImVec2(width, height));

        ImGui::PopStyleColor(3);

        const auto& filename = filepath.stem();
        const auto cursorPosX = ImGui::GetCursorPosX();
        ImGui::SetCursorPosX(cursorPosX + framePadding.x);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 1.5f * style.ItemSpacing.y);
        if (isDirectory)
        {
            const auto textWidth = ImGui::CalcTextSize(filename.c_str()).x;
            const auto aWidth = ImGui::CalcTextSize("a").x;
            if (textWidth > fullWidth)
            {
                uint32_t characters = width / aWidth;
                ImGui::Text("%.*s%s", characters, filename.c_str(), "...");
            }
            else
            {
                ImGui::SetCursorPosX(glm::max(cursorPosX + framePadding.x, cursorPosX + (fullWidth - textWidth) * 0.5f));
                ImGui::Text("%s", filename.c_str());
            }
        }
        else
        {
            ImGui::TextWrapped("%s", filename.c_str());
        }
        ImGui::EndGroup();

        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Delete"))
                FL_LOG("Delete");
            ImGui::EndMenu();
        }

        if (ImGui::BeginDragDropSource())
        {
            ImGui::SetDragDropPayload(
                "FL_CONTENT_BROWSER_ITEM",
                filepath.c_str(),
                (strlen(filepath.c_str()) + 1) * sizeof(char),
                ImGuiCond_Once
            );
            ImGui::Text("%s", filepath.c_str());
            ImGui::EndDragDropSource();
        }
        return ImVec2(fullWidth, fullHeight);
    }

    void UI::Vec3Control(const std::string& str_id, glm::vec3& value, float defaultValue, float dragSpeed, float availWidth)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0);

        float lineHeight = ImGui::GetTextLineHeight() + 2.0f * ImGui::GetStyle().FramePadding.y;
        ImVec2 buttonSize = { 5.0f, lineHeight };

        ImGui::PushID(str_id.c_str());

        ImGui::PushMultiItemsWidths(3, ceil(availWidth + 7.0f - 3 * buttonSize.x));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });

        if (ImGui::Button("##X_Button", buttonSize))
            value.x = defaultValue;
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::DragFloat("##X", &value.x, dragSpeed, 0.0f, 0.0f, "%.2f");
        ImGui::PopItemWidth();
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });

        if (ImGui::Button("##Y_Button", buttonSize))
            value.y = defaultValue;
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::DragFloat("##Y", &value.y, dragSpeed, 0.0f, 0.0f, "%.2f");
        ImGui::PopItemWidth();
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });

        if (ImGui::Button("##Z_Button", buttonSize))
            value.z = defaultValue;
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::DragFloat("##Z", &value.z, dragSpeed, 0.0f, 0.0f, "%.2f");
        ImGui::PopItemWidth();
        ImGui::PopStyleVar();

        ImGui::PopID();
        ImGui::PopStyleVar();
    }

}
