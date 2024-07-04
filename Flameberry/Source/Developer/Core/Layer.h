#pragma once

#include "Event.h"

namespace Flameberry {

	class Layer
	{
	public:
		virtual ~Layer() = default;

		virtual void OnCreate() = 0;
		virtual void OnUpdate(float delta) = 0;
		virtual void OnUIRender() = 0;
		virtual void OnEvent(Event& e) = 0;
		virtual void OnDestroy() = 0;
	};

} // namespace Flameberry