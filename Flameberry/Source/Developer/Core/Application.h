#pragma once

#include <memory>

#include "ImGui/ImGuiLayer.h"
#include "Layer.h"
#include "Window.h"

#include "Renderer/VulkanContext.h"

namespace Flameberry {

	struct ApplicationCommandLineArgs
	{
		int Count;
		const char** Args;

		const char* operator[](int idx) const
		{
			FBY_ASSERT(idx < Count,
				"Command Line Arguments: Index '{}' is out of range!", idx);
			return Args[idx];
		}
	};

	struct ApplicationSpecification
	{
		std::string Name;
		WindowSpecification WindowSpec;
		std::filesystem::path WorkingDirectory;
		ApplicationCommandLineArgs CommandLineArgs;
	};

	class Application
	{
	public:
		Application(const ApplicationSpecification& specification);
		~Application();
		void Run();

		Window& GetWindow() { return *m_Window; }
		static Application& Get() { return *s_Instance; }
		static Application*
		CreateClientApp(const ApplicationCommandLineArgs& appCmdLineArgs);

		void OnEvent(Event& e);
		void OnKeyPressedEvent(KeyPressedEvent& e);
		void OnWindowResizedEvent(WindowResizedEvent& e);

		void ImGuiLayerBlockEvents(bool block) { m_ImGuiLayer->BlockEvents(block); }
		void BlockAllEvents(bool block) { m_BlockAllLayerEvents = block; }

		void PushLayer(Layer* layer);
		void PopLayer(Layer* layer);
		void PopAndDeleteLayer(Layer* layer);

		void PushOverlay(Layer* layer);
		void PopOverlay(Layer* layer);
		void PopAndDeleteOverlay(Layer* layer);

	private:
		ApplicationSpecification m_Specification;

		Ref<Window> m_Window;
		Ref<VulkanContext> m_VulkanContext;
		ImGuiLayer* m_ImGuiLayer;

		bool m_BlockAllLayerEvents = false;

		// Layer Stack Related Variables
		std::vector<Layer*> m_LayerStack;
		uint32_t m_LayerInsertIndex = 0;

	private:
		static Application* s_Instance;
	};
} // namespace Flameberry
