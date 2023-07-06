#include "MaterialSelectorPanel.h"

namespace Flameberry {
    void MaterialSelectorPanel::OpenPanel(const CallBackType& selectCallBack)
    {
        m_Open = true;
        m_SelectCallBack = selectCallBack;
    }

    void MaterialSelectorPanel::OnUIRender()
    {
        if (m_Open)
        {
            ImGui::Begin("Material Finder", &m_Open);
            for (const auto& [uuid, asset] : AssetManager::GetAssetTable())
            {
                if (asset->GetAssetType() == AssetType::Material)
                {
                    auto mat = std::static_pointer_cast<Material>(asset);
                    ImGui::Text("%s", mat->Name.c_str());
                    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                    {
                        m_Open = false;
                        m_SelectedMaterial = mat;

                        m_SelectCallBack(m_SelectedMaterial);
                    }
                }
            }
            ImGui::End();
        }
    }
}