#pragma once

#include <imgui.h>
#include <imgui/imgui_internal.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Core/Application.h"
#include "Core/Core.h"

#include "Core/Input.h"

#ifdef FL_USE_OPENGL_API
#include "Renderer/OpenGL/OpenGLRenderer2D.h"
#include "Renderer/OpenGL/OpenGLRenderCommand.h"
#include "Renderer/OpenGL/OpenGLRenderer3D.h"
#include "Renderer/OpenGL/OpenGLFramebuffer.h"

#include "ECS/Scene.h"
#elif defined(FL_USE_VULKAN_API)
#include "Renderer/Vulkan/VulkanRenderer.h"
#include "Renderer/Vulkan/VulkanRenderCommand.h"
#include "Renderer/Vulkan/VulkanPipeline.h"
#include "Renderer/Vulkan/VulkanDescriptor.h"
#include "Renderer/Vulkan/VulkanTexture.h"
#include "Renderer/Vulkan/VulkanMesh.h"
#include "Renderer/Vulkan/MeshRenderer.h"
#endif

#include "Core/Timer.h"
