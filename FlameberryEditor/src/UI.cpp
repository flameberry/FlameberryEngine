#include "UI.h"

#include <imgui.h>
#include <imgui/imgui_internal.h>
#include <IconFontCppHeaders/IconsLucide.h>

#include "Core/Core.h"
#include "Core/Algorithm.h"

#include "ImGui/Theme.h"

namespace Flameberry {

    struct UIState
    {
        // Selection Widget
        bool IsSelectionWidgetJustOpened = false, IsSelectionWidgetSearchBoxFocused = false, HasSelectionWidgetListBoxBegun = false;
    };

    static UIState g_UIState;

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

    bool UI::AlignedButton(const char* label, const ImVec2& size, float alignment)
    {
        ImGuiStyle& style = ImGui::GetStyle();

        float width = size.x ? size.x : ImGui::CalcTextSize(label).x + style.FramePadding.x * 2.0f;
        float avail = ImGui::GetContentRegionAvail().x;

        float off = (avail - width) * alignment;
        if (off > 0.0f)
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);

        return ImGui::Button(label, size);
    }

    void UI::InputBox(const char* label, const float width, char* inputBuffer, const uint32_t inputLength, const char* inputHint)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5, 3));
        ImGui::PushItemWidth(width);
        ImGui::InputTextWithHint(label, inputHint, inputBuffer, inputLength);
        ImGui::PopItemWidth();
        ImGui::PopStyleVar(2);
    }

    void UI::OpenSelectionWidget(const char* label)
    {
        std::string labelFmt = fmt::format("{}Popup", label);
        ImGui::OpenPopup(labelFmt.c_str());

        g_UIState.IsSelectionWidgetJustOpened = true;
    }

    bool UI::BeginSelectionWidget(const char* label, char* inputBuffer, const uint32_t inputLength)
    {
        std::string labelFmt = fmt::format("{}Popup", label);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(2, 2));
        bool beginPopup = ImGui::BeginPopup(labelFmt.c_str());
        ImGui::PopStyleVar();

        if (beginPopup)
        {
            if (g_UIState.IsSelectionWidgetJustOpened)
            {
                ImGui::SetKeyboardFocusHere();
                g_UIState.IsSelectionWidgetJustOpened = false;
            }

            if (g_UIState.IsSelectionWidgetSearchBoxFocused)
            {
                ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
                ImGui::PushStyleColor(ImGuiCol_Border, ImVec4{ 254.0f / 255.0f, 211.0f / 255.0f, 140.0f / 255.0f, 1.0f });
            }

            std::string inputBoxLabel = fmt::format("{}SearchBar", label);
            UI::InputBox(inputBoxLabel.c_str(), -1.0f, inputBuffer, inputLength, ICON_LC_SEARCH" Search...");

            if (g_UIState.IsSelectionWidgetSearchBoxFocused)
            {
                ImGui::PopStyleColor();
                ImGui::PopStyleVar();
            }

            g_UIState.IsSelectionWidgetSearchBoxFocused = ImGui::IsItemActive() && ImGui::IsItemFocused();
            g_UIState.HasSelectionWidgetListBoxBegun = ImGui::BeginListBox("##ScriptActorClassesListBox");
        }

        return beginPopup;
    }

    bool UI::SelectionWidgetElement(const char* label, bool isSelected)
    {
        if (ImGui::Selectable(label, &isSelected))
        {
            ImGui::CloseCurrentPopup();
            return true;
        }

        if (isSelected)
            ImGui::SetItemDefaultFocus();

        return false;
    }

    void UI::EndSelectionWidget()
    {
        if (g_UIState.HasSelectionWidgetListBoxBegun)
            ImGui::EndListBox();

        ImGui::EndPopup();
    }

    // TODO: Call one of 2 functions one for Folder Thumbnail and other for file
    bool UI::ContentBrowserItem(const std::filesystem::path& filepath, float size, const Ref<Texture2D>& thumbnail, ImVec2& outItemSize, bool keepExtension)
    {
        bool isDirectory = std::filesystem::is_directory(filepath);

        ImGuiStyle& style = ImGui::GetStyle();

        const auto& specification = thumbnail->GetImageSpecification();
        const float aspectRatio = (float)specification.Width / (float)specification.Height;

        const float width = size;
        float height = size;

        const float borderThickness = 1.5f;

        const float thumbnailWidth = specification.Width >= specification.Height ? size - 2.0f * borderThickness : height * aspectRatio;
        const float thumbnailHeight = specification.Width >= specification.Height ? width / aspectRatio : size - 2.0f * borderThickness;

        const auto& framePadding = style.FramePadding;
        height += framePadding.y;

        const float textHeight = ImGui::GetTextLineHeightWithSpacing();
        const float fullWidth = width;
        const float fullHeight = height + 2 * textHeight;

        const auto& cursorPos = ImGui::GetCursorScreenPos();
        bool hovered, held;
        bool isDoubleClicked = ImGui::ButtonBehavior(ImRect(cursorPos, cursorPos + ImVec2(fullWidth, fullHeight)), ImGui::GetID(filepath.c_str()), &hovered, &held, ImGuiButtonFlags_PressedOnDoubleClick);

        if (!isDirectory)
        {
            ImGui::GetWindowDrawList()->AddRectFilled(cursorPos, cursorPos + ImVec2(fullWidth, height), 0xff151515, 3, ImDrawFlags_RoundCornersTopLeft | ImDrawFlags_RoundCornersTopRight);
            ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(cursorPos.x, cursorPos.y + height), cursorPos + ImVec2(fullWidth, fullHeight), 0xff353535, 3, ImDrawFlags_RoundCornersBottomLeft | ImDrawFlags_RoundCornersBottomRight);
            ImGui::GetWindowDrawList()->AddRect(cursorPos, cursorPos + ImVec2(fullWidth, fullHeight), hovered ? ImGui::ColorConvertFloat4ToU32(Theme::AccentColor) : 0xff000000, 3, 0, borderThickness);
        }
        else if (hovered)
        {
            ImGui::GetWindowDrawList()->AddRectFilled(cursorPos, cursorPos + ImVec2(fullWidth, fullHeight), IM_COL32(60, 60, 60, 255), 3);
        }

        ImGui::BeginGroup();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));

        const float centerTranslationWidth = width / 2.0f - thumbnailWidth / 2.0f;
        const float centerTranslationHeight = height / 2.0f - thumbnailHeight / 2.0f;

        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + centerTranslationWidth - framePadding.x);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + centerTranslationHeight - framePadding.y);

        ImGui::ImageButton(filepath.c_str(), reinterpret_cast<ImTextureID>(thumbnail->CreateOrGetDescriptorSet()), ImVec2(thumbnailWidth, thumbnailHeight));

        ImGui::PopStyleColor(3);

        const auto& filename = keepExtension ? filepath.filename() : filepath.stem();
        const auto cursorPosX = ImGui::GetCursorPosX();
        ImGui::SetCursorPosX(cursorPosX + framePadding.x);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - style.ItemSpacing.y + centerTranslationHeight);

        const auto textWidth = ImGui::CalcTextSize(filename.c_str()).x;
        const auto aWidth = ImGui::CalcTextSize("a").x;
        const uint32_t characters = fullWidth / aWidth;

        // Format and align text based on whether the item is a directory or a file
        if (isDirectory)
        {
            if (textWidth > fullWidth)
                ImGui::Text("%.*s%s", characters, filename.c_str(), "...");
            else
            {
                ImGui::SetCursorPosX(glm::max(cursorPosX + framePadding.x, cursorPosX + (fullWidth - textWidth) * 0.5f));
                ImGui::Text("%s", filename.c_str());
            }
        }
        else
        {
            if (textWidth > 2.0f * fullWidth)
                ImGui::TextWrapped("%.*s%s", 2 * characters, filename.c_str(), "...");
            else
                ImGui::TextWrapped("%s", filename.c_str());
        }

        ImGui::EndGroup();

        if (ImGui::BeginPopupContextItem(filepath.c_str()))
        {
            if (ImGui::MenuItem(ICON_LC_DELETE"\tDelete"))
            {
                // Add a confirm popup
                // std::filesystem::remove(filepath);
                FBY_LOG("Delete");
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginDragDropSource())
        {
            ImGui::SetDragDropPayload(
                "FBY_CONTENT_BROWSER_ITEM",
                filepath.c_str(),
                (strlen(filepath.c_str()) + 1) * sizeof(char),
                ImGuiCond_Once
            );
            ImGui::Text("%s", filepath.c_str());
            ImGui::EndDragDropSource();
        }
        outItemSize = ImVec2(fullWidth, fullHeight);
        return isDoubleClicked;
    }

    bool UI::ProjectRegistryEntryItem(const char* projectName, const char* path, bool disabled)
    {
        constexpr float paddingX = 15.0f, paddingY = 5.0f, spacing = 10.0f;
        const float itemWidth = ImGui::GetContentRegionAvail().x;

        ImGui::SetNextItemWidth(itemWidth);

        if (disabled)
            ImGui::BeginDisabled();

        const auto& cursorScreenPos = ImGui::GetCursorScreenPos();

        ImGui::BeginGroup();
        ImVec2 cursorPos = ImGui::GetCursorPos();
        ImGui::SetCursorPosX(cursorPos.x + paddingX);
        ImGui::SetCursorPosY(cursorPos.y + 2.0f * paddingY);

        auto& bigFont = ImGui::GetIO().Fonts->Fonts[0];
        ImGui::Text("%s", projectName);

        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + paddingX);
        ImGui::TextWrapped("%s", path);

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + paddingY);

        ImGui::EndGroup();

        if (disabled)
            ImGui::EndDisabled();

        ImRect itemRect(cursorScreenPos, cursorScreenPos + ImVec2(itemWidth, ImGui::GetCursorPosY() - cursorPos.y));
        bool hovered, held;
        bool isDoubleClicked = ImGui::ButtonBehavior(itemRect, ImGui::GetID(projectName), &hovered, &held, ImGuiButtonFlags_PressedOnDoubleClick);

        if (hovered)
        {
            const ImU32 color = ImGui::IsMouseDown(0) ? IM_COL32(255, 255, 255, 60) : IM_COL32(255, 255, 255, 30);
            ImGui::GetWindowDrawList()->AddRectFilled(itemRect.Min, itemRect.Max, color, 5.0f);
        }
        return isDoubleClicked;
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
