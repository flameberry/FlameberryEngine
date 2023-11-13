#pragma once

#include <vulkan/vulkan.h>

#include <imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/backends/imgui_impl_vulkan.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Core/Application.h"
#include "Core/Core.h"
#include "Core/Event.h"
#include "Core/Layer.h"
#include "Core/Timer.h"
#include "Core/Input.h"
#include "Core/Math.h"
#include "Core/Timer.h"
#include "Core/Profiler.h"
#include "Core/PatternMatching.h"

#include "Platform/PlatformUtils.h"
#include "ImGui/ImGuiLayer.h"
#include "Asset/AssetManager.h"

#include "Renderer/RenderCommand.h"
#include "Renderer/Pipeline.h"
#include "Renderer/DescriptorSet.h"
#include "Renderer/Texture2D.h"
#include "Renderer/StaticMesh.h"
#include "Renderer/VulkanDebug.h"
#include "Renderer/RenderPass.h"
#include "Renderer/SceneRenderer.h"
#include "Renderer/Renderer2D.h"
#include "Renderer/Renderer.h"
#include "Renderer/EditorCameraController.h"

#include "ECS/Components.h"
#include "ECS/SceneSerializer.h"
#include "ECS/Scene.h"
#include "ECS/ecs.hpp"

#include "Project/Project.h"

#include "Scripting/ScriptEngine.h"
