#pragma once

#include "Flameberry.h"

#include "Panels/SceneHierarchyPanel.h"
#include "Panels/ContentBrowserPanel.h"
#include "Panels/InspectorPanel.h"
#include "Core/UI.h"

namespace Flameberry {

	enum class EditorState : uint8_t
	{
		Edit = 0,
		Play = 1,
		Simulate = 2,
	};

	class EditorLayer : public Layer
	{
	public:
		EditorLayer(const Ref<Project>& project);
		virtual ~EditorLayer() = default;

		void OnCreate() override;
		void OnUpdate(float delta) override;
		void OnUIRender() override;
		void OnEvent(Event& e) override;
		void OnDestroy() override;

		void OnKeyPressedEvent(KeyPressedEvent& e);
		void OnMouseButtonPressedEvent(MouseButtonPressedEvent& e);
		void OnMouseScrolledEvent(MouseScrollEvent& e);

		void InvalidateViewportImGuiDescriptorSet(uint32_t index);
		void InvalidateCompositePassImGuiDescriptorSet(uint32_t index);

		void OpenProject();
		void OpenProject(const std::string& path);
		void SaveScene();
		void SaveSceneAs();
		void OpenScene(AssetHandle handle);
		void OpenScene(const std::string& path);
		void OpenScene();
		void NewScene();
		void SetActiveScene(const Ref<Scene>& scene);

		void OnSceneEdit();
		void OnScenePlay();
		void OnSceneSimulate();

		void UI_Menubar();
		void UI_Toolbar();
		void UI_CompositeView();
		void UI_RendererSettings();
		void UI_AssetRegistry();
		void UI_GizmoOverlay(const ImVec2& workPos);
		void UI_ToolbarOverlay(const ImVec2& workPos, const ImVec2& workSize);
		void UI_ViewportSettingsOverlay(const ImVec2& workPos, const ImVec2& workSize);
		void UI_BottomPanel();

		// static_assert(std::is_invocable_v<Fn>);
		template <typename Fn>
		void UI_Overlay(const char* str_id, const ImVec2& position, Fn&& func);

	private:
		EditorCameraController m_ActiveCameraController;

		// Test
		bool m_ShouldReloadMeshShaders = false;

		// Scalars
		EditorState m_EditorState = EditorState::Edit;
		int m_MouseX = 0, m_MouseY = 0;
		int m_GizmoType = -1;
		bool m_IsViewportFocused = false, m_IsViewportHovered = false, m_HasViewportSizeChanged = false;
		bool m_IsCameraMoving = false, m_IsGizmoActive = false;
		bool m_IsMousePickingBufferReady = false, m_DidViewportBegin = true, m_IsAnyOverlayHovered = false;
		bool m_EnableGrid = true;

		glm::vec2 m_ViewportSize{ 1280, 720 };
		glm::vec2 m_RenderViewportSize{ 1280, 720 }; // This is to store the viewport size to be rendered to for high dpi/retina displays
		glm::vec2 m_ViewportBounds[2];

		std::string m_EditorScenePath = "", m_ScenePathToBeOpened = "";
		bool m_ShouldOpenAnotherScene = false;

		// Texture Icons
		Ref<Texture2D> m_CursorIcon, m_TranslateIcon, m_RotateIcon, m_ScaleIcon, m_PlayAndStopIcon, m_SettingsIcon, m_PauseIcon, m_StepIcon, m_SimulateAndPauseIcon;
		static constexpr ImVec2 s_OverlayButtonSize = ImVec2(16, 16);
		static constexpr float s_OverlayPadding = 6.0f;

		// Renderer
		Unique<SceneRenderer> m_SceneRenderer;

		// ECS
		Ref<Scene> m_ActiveScene, m_ActiveSceneBackUpCopy;

		std::vector<VkDescriptorSet> m_ViewportDescriptorSets;
		std::vector<VkDescriptorSet> m_CompositePassViewportDescriptorSets;

		// UI
		Ref<SceneHierarchyPanel> m_SceneHierarchyPanel;
		Ref<ContentBrowserPanel> m_ContentBrowserPanel;

		// Mouse Picking
		Unique<Buffer> m_MousePickingBuffer;
		Ref<RenderPass> m_MousePickingRenderPass;

		Ref<Pipeline> m_MousePickingPipeline, m_MousePicking2DPipeline;
		Ref<DescriptorSetLayout> m_MousePickingDescriptorSetLayout;

		Ref<Project> m_Project;
	};

	template <typename Fn>
	void EditorLayer::UI_Overlay(const char* str_id, const ImVec2& position, Fn&& func)
	{
		if (m_DidViewportBegin)
		{
			ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration
				| ImGuiWindowFlags_AlwaysAutoResize
				| ImGuiWindowFlags_NoSavedSettings
				| ImGuiWindowFlags_NoFocusOnAppearing
				| ImGuiWindowFlags_NoNav;

			ImGui::SetNextWindowPos(position, ImGuiCond_Always);
			window_flags |= ImGuiWindowFlags_NoMove;

			ImGui::SetNextWindowBgAlpha(0.45f); // Transparent background

			// TODO: This seems to have no effect
			ImGuiWindowClass windowClass;
			windowClass.ViewportFlagsOverrideSet = ImGuiViewportFlags_TopMost;
			ImGui::SetNextWindowClass(&windowClass);

			{
				UI::ScopedStyleVariable windowMinSize(ImGuiStyleVar_WindowMinSize, ImVec2(20, 20));
				UI::ScopedStyleVariable windowBorderSize(ImGuiStyleVar_WindowBorderSize, 0.0f);
				UI::ScopedStyleVariable windowRounding(ImGuiStyleVar_WindowRounding, 16.0f);
				UI::ScopedStyleVariable windowPadding(ImGuiStyleVar_WindowPadding, ImVec2{ 5.0f, 3.0f });

				ImGui::Begin(str_id, nullptr, window_flags);
			}

			m_IsAnyOverlayHovered = m_IsAnyOverlayHovered || ImGui::IsWindowHovered();

			{
				UI::ScopedStyleVariable itemSpacing(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });
				UI::ScopedStyleColor button(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
				UI::ScopedStyleColor buttonActive(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));

				func();
			}

			ImGui::End();
		}
	}

} // namespace Flameberry
