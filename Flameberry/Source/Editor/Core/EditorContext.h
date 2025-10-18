#pragma once

#include "ECS/ecs.hpp"

namespace Flameberry {

	struct EditorContext
	{
		FEntity SelectedEntity;

		static void Create();
		static void Destroy();
		static EditorContext* Get() { return s_Ctx; }

	private:
		static EditorContext* s_Ctx;
	};

} // namespace Flameberry
