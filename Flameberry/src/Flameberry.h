#pragma once

#include <imgui.h>
#include <imgui/imgui_internal.h>

#include <glm/gtc/type_ptr.hpp>

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
#endif

#include "Renderer/ModelLoader.h"

#include "Core/Timer.h"
