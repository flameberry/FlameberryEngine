#pragma once

#include <imgui.h>
#include <imgui/imgui_internal.h>

#include "Core/Application.h"
#include "Core/Core.h"

#ifdef FL_USE_OPENGL_API
#include "Renderer/OpenGL/OpenGLRenderer2D.h"
#include "Renderer/OpenGL/OpenGLRenderer3D.h"
#include "Renderer/OpenGL/OpenGLFramebuffer.h"

#include "ECS/Scene.h"
#endif

#include "Core/Timer.h"
