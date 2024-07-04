#pragma once

#include <vulkan/vulkan.h>

#include <imgui.h>
#include <imgui/backends/imgui_impl_vulkan.h>
#include <imgui/imgui_internal.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Core/Algorithm.h"
#include "Core/Application.h"
#include "Core/Core.h"
#include "Core/Event.h"
#include "Core/Input.h"
#include "Core/Layer.h"
#include "Core/Math.h"
#include "Core/Profiler.h"
#include "Core/Timer.h"

#include "Platform/PlatformUtils.h"

#include "ImGui/ImGuiLayer.h"
#include "ImGui/Theme.h"

#include "Asset/AssetManager.h"

#include "Renderer/DescriptorSet.h"
#include "Renderer/EditorCameraController.h"
#include "Renderer/Pipeline.h"
#include "Renderer/RenderCommand.h"
#include "Renderer/RenderPass.h"
#include "Renderer/Renderer.h"
#include "Renderer/Renderer2D.h"
#include "Renderer/SceneRenderer.h"
#include "Renderer/StaticMesh.h"
#include "Renderer/Texture2D.h"
#include "Renderer/VulkanDebug.h"

#include "ECS/Components.h"
#include "ECS/Scene.h"
#include "ECS/SceneSerializer.h"
#include "ECS/ecs.hpp"

#include "Project/Project.h"

#include "Scripting/ScriptEngine.h"
