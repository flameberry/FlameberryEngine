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
#include "ImGui/ImGuiLayer.h"

#include "Renderer/Vulkan/VulkanRenderer.h"
#include "Renderer/Vulkan/VulkanRenderCommand.h"
#include "Renderer/Vulkan/VulkanPipeline.h"
#include "Renderer/Vulkan/VulkanDescriptor.h"
#include "Renderer/Vulkan/VulkanTexture.h"
#include "Renderer/Vulkan/VulkanMesh.h"
#include "Renderer/Vulkan/VulkanDebug.h"
#include "Renderer/Vulkan/SceneRenderer.h"
#include "Renderer/SkyboxRenderer.h"

#include "ECS/Component.h"
#include "ECS/SceneSerializer.h"
#include "ECS/Scene.h"
#include "ECS/ecs.hpp"

#include "Core/Timer.h"
#include "Core/Profiler.h"

#include "Platform/Utils.h"
